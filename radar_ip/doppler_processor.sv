module doppler_processor #(
    parameter DATA_WIDTH = 16,
    parameter DOPPLER_SIZE = 64
)(
    input wire clk,
    input wire rst_n,
    input wire enable,
    input wire [DATA_WIDTH-1:0] data_in,
    input wire data_valid,
    input wire [31:0] doppler_bins,
    output wire [DATA_WIDTH-1:0] processed_data,
    output wire processed_valid
);

// Buffer for collecting pulses across range gates
reg [DATA_WIDTH-1:0] pulse_buffer [0:DOPPLER_SIZE-1];
reg [$clog2(DOPPLER_SIZE)-1:0] pulse_count;
reg buffer_full;

// Window function for Doppler processing
wire [DATA_WIDTH-1:0] windowed_doppler_data;
wire windowed_doppler_valid;

// FFT for Doppler processing
wire [DATA_WIDTH-1:0] doppler_fft_real, doppler_fft_imag;
wire doppler_fft_valid;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        pulse_count <= 0;
        buffer_full <= 0;
    end else if (enable && data_valid) begin
        pulse_buffer[pulse_count] <= data_in;
        
        if (pulse_count == DOPPLER_SIZE - 1) begin
            pulse_count <= 0;
            buffer_full <= 1;
        end else begin
            pulse_count <= pulse_count + 1;
            buffer_full <= 0;
        end
    end
end

// Doppler windowing (Hamming window)
hamming_window #(
    .DATA_WIDTH(DATA_WIDTH),
    .WINDOW_SIZE(DOPPLER_SIZE)
) u_doppler_window (
    .clk(clk),
    .rst_n(rst_n),
    .data_in(pulse_buffer[pulse_count]),
    .data_valid(buffer_full),
    .data_out(windowed_doppler_data),
    .data_out_valid(windowed_doppler_valid)
);

// Doppler FFT
fft_processor #(
    .DATA_WIDTH(DATA_WIDTH),
    .FFT_SIZE(DOPPLER_SIZE)
) u_doppler_fft (
    .clk(clk),
    .rst_n(rst_n),
    .enable(enable),
    .data_in(windowed_doppler_data),
    .data_valid(windowed_doppler_valid),
    .fft_real(doppler_fft_real),
    .fft_imag(doppler_fft_imag),
    .fft_valid(doppler_fft_valid)
);

// Magnitude calculation for Doppler spectrum
magnitude_calc #(
    .DATA_WIDTH(DATA_WIDTH)
) u_doppler_magnitude (
    .clk(clk),
    .rst_n(rst_n),
    .real_in(doppler_fft_real),
    .imag_in(doppler_fft_imag),
    .valid_in(doppler_fft_valid),
    .magnitude_out(processed_data),
    .valid_out(processed_valid)
);

endmodule
