module hamming_window #(
    parameter DATA_WIDTH = 16,
    parameter WINDOW_SIZE = 1024
)(
    input wire clk,
    input wire rst_n,
    input wire [DATA_WIDTH-1:0] data_in,
    input wire data_valid,
    output wire [DATA_WIDTH-1:0] data_out,
    output wire data_out_valid
);

// Pre-computed Hamming window coefficients (stored in ROM)
reg [DATA_WIDTH-1:0] window_coeff [0:WINDOW_SIZE-1];
reg [$clog2(WINDOW_SIZE)-1:0] window_index;
reg [DATA_WIDTH*2-1:0] multiplied_result;
reg result_valid;

// Initialize Hamming window coefficients
initial begin
    for (int i = 0; i < WINDOW_SIZE; i++) begin
        // Hamming window: 0.54 - 0.46 * cos(2*pi*i/(N-1))
        window_coeff[i] = $rtoi((0.54 - 0.46 * $cos(2.0 * 3.14159 * i / (WINDOW_SIZE - 1))) * ((2**(DATA_WIDTH-1)) - 1));
    end
end

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        window_index <= 0;
        multiplied_result <= 0;
        result_valid <= 0;
    end else if (data_valid) begin
        // Multiply input data with window coefficient
        multiplied_result <= data_in * window_coeff[window_index];
        result_valid <= 1;
        
        // Update window index
        if (window_index == WINDOW_SIZE - 1) begin
            window_index <= 0;
        end else begin
            window_index <= window_index + 1;
        end
    end else begin
        result_valid <= 0;
    end
end

assign data_out = multiplied_result[DATA_WIDTH*2-1:DATA_WIDTH];
assign data_out_valid = result_valid;

endmodule
