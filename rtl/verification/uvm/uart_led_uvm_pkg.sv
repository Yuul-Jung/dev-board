package uart_led_uvm_pkg;
 import uvm_pkg::*;
 `include "uvm_macros.svh"

 class led_item extends uvm_sequence_item;
   rand bit [7:0] data;
   rand bit valid;
   `uvm_object_utils(led_item)
   function new(string name="led_item"); super.new(name); endfunction
 endclass

 class led_sequencer extends uvm_sequencer #(led_item);
   `uvm_component_utils(led_sequencer)
   function new(string n,uvm_component p); super.new(n,p); endfunction
 endclass

 class led_driver extends uvm_driver #(led_item);
   `uvm_component_utils(led_driver)
   virtual uart_led_if vif;
   function new(string n,uvm_component p); super.new(n,p); endfunction
   function void build_phase(uvm_phase phase);
     super.build_phase(phase);
     if(!uvm_config_db#(virtual uart_led_if)::get(this,"","vif",vif))
       `uvm_fatal("NOVIF","driver needs uart_led_if")
   endfunction
   task run_phase(uvm_phase phase);
     led_item t;
     vif.rx_valid=0;
     vif.rx_data=0;
     forever begin
       seq_item_port.get_next_item(t);
       @(negedge vif.clk);
       vif.rx_data=t.data;
       vif.rx_valid=t.valid;
       @(posedge vif.clk);
       @(negedge vif.clk);
       vif.rx_valid=0;
       seq_item_port.item_done();
     end
   endtask
 endclass

 class led_monitor extends uvm_component;
   `uvm_component_utils(led_monitor)
   virtual uart_led_if vif;
   uvm_analysis_port #(led_item) ap;
   function new(string n,uvm_component p);
     super.new(n,p); ap=new("ap",this);
   endfunction
   function void build_phase(uvm_phase phase);
     if(!uvm_config_db#(virtual uart_led_if)::get(this,"","vif",vif))
       `uvm_fatal("NOVIF","monitor needs uart_led_if")
   endfunction
   task run_phase(uvm_phase phase);
     led_item t;
     forever begin
       @(posedge vif.clk);
       if(!vif.rst && vif.rx_valid && vif.rx_ready) begin
         t=led_item::type_id::create("observed");
         t.data=vif.rx_data;
         t.valid=1;
         ap.write(t);
       end
     end
   endtask
 endclass

 class led_scoreboard extends uvm_subscriber #(led_item);
   `uvm_component_utils(led_scoreboard)
   virtual uart_led_if vif;
   bit expected;
   int checked;
   function new(string n,uvm_component p); super.new(n,p); endfunction
   function void build_phase(uvm_phase phase);
     if(!uvm_config_db#(virtual uart_led_if)::get(this,"","vif",vif))
       `uvm_fatal("NOVIF","scoreboard needs uart_led_if")
   endfunction
   function void write(led_item t);
     case(t.data)
      8'h31:expected=1;
      8'h30:expected=0;
      default:begin end
     endcase
     checked++;
   endfunction
   task run_phase(uvm_phase phase);
     forever begin
       @(negedge vif.clk);
       if(vif.rst) expected=0;
       else if(vif.led !== expected)
         `uvm_error("MISMATCH",$sformatf("LED expected=%0b actual=%0b",expected,vif.led))
     end
   endtask
   function void report_phase(uvm_phase phase);
     if(checked<4) `uvm_error("COVERAGE","Too few byte transfers")
     else `uvm_info("UVM_SCOREBOARD_PASS",$sformatf("checked %0d transfers",checked),UVM_NONE)
   endfunction
 endclass

 class led_agent extends uvm_agent;
   `uvm_component_utils(led_agent)
   led_driver driver;
   led_monitor monitor;
   led_sequencer sequencer;
   function new(string n,uvm_component p); super.new(n,p); endfunction
   function void build_phase(uvm_phase phase);
     driver=led_driver::type_id::create("driver",this);
     monitor=led_monitor::type_id::create("monitor",this);
     sequencer=led_sequencer::type_id::create("sequencer",this);
   endfunction
   function void connect_phase(uvm_phase phase);
     driver.seq_item_port.connect(sequencer.seq_item_export);
   endfunction
 endclass

 class led_env extends uvm_env;
   `uvm_component_utils(led_env)
   led_agent agent;
   led_scoreboard scoreboard;
   function new(string n,uvm_component p);super.new(n,p);endfunction
   function void build_phase(uvm_phase phase);
     agent=led_agent::type_id::create("agent",this);
     scoreboard=led_scoreboard::type_id::create("scoreboard",this);
   endfunction
   function void connect_phase(uvm_phase phase);
     agent.monitor.ap.connect(scoreboard.analysis_export);
   endfunction
 endclass

 class led_sequence extends uvm_sequence #(led_item);
   `uvm_object_utils(led_sequence)
   function new(string n="led_sequence");super.new(n);endfunction
   task body();
     led_item t;
     bit [7:0] vals[5]='{8'h31,8'h58,8'h30,8'h31,8'h30};
     foreach(vals[i]) begin
       t=led_item::type_id::create("t");
       start_item(t);
       t.data=vals[i]; t.valid=1;
       finish_item(t);
     end
   endtask
 endclass

 class led_test extends uvm_test;
   `uvm_component_utils(led_test)
   led_env env;
   function new(string n,uvm_component p);super.new(n,p);endfunction
   function void build_phase(uvm_phase phase);
     env=led_env::type_id::create("env",this);
   endfunction
   task run_phase(uvm_phase phase);
     led_sequence seq;
     phase.raise_objection(this);
     seq=led_sequence::type_id::create("seq");
     seq.start(env.agent.sequencer);
     repeat(5) @(negedge env.agent.scoreboard.vif.clk);
     phase.drop_objection(this);
   endtask
 endclass
endpackage
