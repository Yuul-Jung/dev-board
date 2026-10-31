module network_sample_register(input wire clk,rst,input wire [15:0]sample,input wire sample_valid,output wire sample_ready,output reg [15:0]payload,output reg payload_valid,input wire payload_ready);
assign sample_ready=!payload_valid||payload_ready;
always @(posedge clk)begin
 if(rst)begin payload<=0;payload_valid<=0;end
 else if(sample_ready)begin
 payload_valid<=sample_valid;
 if(sample_valid)payload<=sample;
 end
end
endmodule
