module pulse_generator (
    input wire clk,
    input wire rst_n,
    input wire enable,
    input wire [31:0] prf_setting,
    input wire [31:0] pulse_width_setting,
    output reg tx_pulse
);

reg [31:0] prf_counter;
reg [31:0] pulse_width_counter;
reg pulse_active;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        prf_counter <= 0;
        pulse_width_counter <= 0;
        pulse_active <= 0;
        tx_pulse <= 0;
    end else if (enable) begin
        // PRF counter
        if (prf_counter >= prf_setting) begin
            prf_counter <= 0;
            pulse_active <= 1;
            pulse_width_counter <= 0;
        end else begin
            prf_counter <= prf_counter + 1;
        end
        
        // Pulse width counter
        if (pulse_active) begin
            if (pulse_width_counter >= pulse_width_setting) begin
                pulse_active <= 0;
                tx_pulse <= 0;
            end else begin
                pulse_width_counter <= pulse_width_counter + 1;
                tx_pulse <= 1;
            end
        end
    end else begin
        tx_pulse <= 0;
        pulse_active <= 0;
    end
end

endmodule
