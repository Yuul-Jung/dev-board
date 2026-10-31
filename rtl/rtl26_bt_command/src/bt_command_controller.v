module bt_command_controller(input wire clk,rst,input wire [7:0]data,input wire valid,output wire ready,output reg led,output reg servo_enable);
assign ready=1'b1;
always @(posedge clk)begin
 if(rst)begin led<=0;servo_enable<=0;end
 else if(valid&&ready)begin
 case(data)
 8'h31:led<=1;8'h30:led<=0;
 8'h53:servo_enable<=1;8'h73:servo_enable<=0;
 default:begin end
 endcase
 end
end
endmodule
