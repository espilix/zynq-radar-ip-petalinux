module mti_filter #(
    parameter DATA_WIDTH = 16
)(
    input wire clk,
    input wire rst_n,
    input wire enable,
    input wire [DATA_WIDTH-1:0] data_in,
    input wire data_valid,
    output wire [DATA_WIDTH-1:0] data_out,
    output wire data_out_valid
);

reg [DATA_WIDTH-1:0] prev_pulse_data;
reg prev_pulse_valid;
reg [DATA_WIDTH-1:0] mti_result;
reg mti_valid;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        prev_pulse_data <= 0;
        prev_pulse_valid <= 0;
        mti_result <= 0;
        mti_valid <= 0;
    end else if (enable && data_valid) begin
        // 2-pulse canceller: current - previous
        if (prev_pulse_valid) begin
            mti_result <= data_in - prev_pulse_data;
            mti_valid <= 1;
        end else begin
            mti_valid <= 0;
        end
        
        // Store current pulse for next iteration
        prev_pulse_data <= data_in;
        prev_pulse_valid <= 1;
    end else begin
        mti_valid <= 0;
    end
end

assign data_out = mti_result;
assign data_out_valid = mti_valid;

endmodule
