module range_processor #(
    parameter ADC_WIDTH = 16,
    parameter FFT_SIZE = 1024
)(
    input wire clk,
    input wire rst_n,
    input wire enable,
    input wire [ADC_WIDTH-1:0] rx_data,
    input wire rx_valid,
    input wire [31:0] range_gates,
    output wire [ADC_WIDTH-1:0] processed_data,
    output wire processed_valid
);

// Window function (Hamming window)
wire [ADC_WIDTH-1:0] windowed_data;
wire windowed_valid;

hamming_window #(
    .DATA_WIDTH(ADC_WIDTH),
    .WINDOW_SIZE(FFT_SIZE)
) u_window (
    .clk(clk),
    .rst_n(rst_n),
    .data_in(rx_data),
    .data_valid(rx_valid),
    .data_out(windowed_data),
    .data_out_valid(windowed_valid)
);

// FFT processor
wire [ADC_WIDTH-1:0] fft_real, fft_imag;
wire fft_valid;

fft_processor #(
    .DATA_WIDTH(ADC_WIDTH),
    .FFT_SIZE(FFT_SIZE)
) u_fft (
    .clk(clk),
    .rst_n(rst_n),
    .enable(enable),
    .data_in(windowed_data),
    .data_valid(windowed_valid),
    .fft_real(fft_real),
    .fft_imag(fft_imag),
    .fft_valid(fft_valid)
);

// Magnitude calculation
magnitude_calc #(
    .DATA_WIDTH(ADC_WIDTH)
) u_magnitude (
    .clk(clk),
    .rst_n(rst_n),
    .real_in(fft_real),
    .imag_in(fft_imag),
    .valid_in(fft_valid),
    .magnitude_out(processed_data),
    .valid_out(processed_valid)
);

endmodule
