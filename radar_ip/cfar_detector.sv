module cfar_detector #(
    parameter DATA_WIDTH = 16,
    parameter GUARD_CELLS = 4,
    parameter REFERENCE_CELLS = 16
)(
    input wire clk,
    input wire rst_n,
    input wire enable,
    input wire [DATA_WIDTH-1:0] data_in,
    input wire data_valid,
    input wire [15:0] threshold_scale,
    output reg [15:0] detected_range,
    output reg [15:0] detected_velocity,
    output reg target_detected
);

// CFAR sliding window
reg [DATA_WIDTH-1:0] cfar_window [0:GUARD_CELLS*2+REFERENCE_CELLS*2];
reg [$clog2(GUARD_CELLS*2+REFERENCE_CELLS*2+1)-1:0] window_ptr;
reg [DATA_WIDTH-1:0] reference_sum;
reg [DATA_WIDTH-1:0] threshold;
reg [DATA_WIDTH-1:0] cell_under_test;
reg [15:0] range_counter;
reg [15:0] velocity_counter;

integer i;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        window_ptr <= 0;
        reference_sum <= 0;
        threshold <= 0;
        target_detected <= 0;
        detected_range <= 0;
        detected_velocity <= 0;
        range_counter <= 0;
        velocity_counter <= 0;
    end else if (enable && data_valid) begin
        // Shift window
        for (i = GUARD_CELLS*2+REFERENCE_CELLS*2; i > 0; i = i - 1) begin
            cfar_window[i] <= cfar_window[i-1];
        end
        cfar_window[0] <= data_in;
        
        // Calculate reference sum (excluding guard cells)
        reference_sum <= 0;
        for (i = 0; i < REFERENCE_CELLS; i = i + 1) begin
            reference_sum <= reference_sum + cfar_window[i];
        end
        for (i = GUARD_CELLS*2+REFERENCE_CELLS; i < GUARD_CELLS*2+REFERENCE_CELLS*2; i = i + 1) begin
            reference_sum <= reference_sum + cfar_window[i];
        end
        
        // Calculate threshold
        threshold <= (reference_sum >> $clog2(REFERENCE_CELLS*2)) * threshold_scale;
        
        // Cell under test
        cell_under_test <= cfar_window[GUARD_CELLS+REFERENCE_CELLS];
        
        // Detection logic
        if (cell_under_test > threshold) begin
            target_detected <= 1;
            detected_range <= range_counter;
            detected_velocity <= velocity_counter;
        end else begin
            target_detected <= 0;
        end
        
        // Update counters
        range_counter <= range_counter + 1;
        if (range_counter == 1023) begin // Assuming 1024 range gates
            range_counter <= 0;
            velocity_counter <= velocity_counter + 1;
            if (velocity_counter == 63) begin // Assuming 64 Doppler bins
                velocity_counter <= 0;
            end
        end
    end
end

endmodule
