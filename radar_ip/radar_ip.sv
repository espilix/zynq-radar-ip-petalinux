module radar_ip #(
    parameter ADC_WIDTH = 16,
    parameter FFT_SIZE = 1024,
    parameter DOPPLER_SIZE = 64,
    parameter AXI_DATA_WIDTH = 32
)(
    // Clock and Reset
    input wire clk,
    input wire rst_n,
    
    // AXI4-Lite Control Interface
    input wire [31:0] s_axi_awaddr,
    input wire s_axi_awvalid,
    output wire s_axi_awready,
    input wire [31:0] s_axi_wdata,
    input wire [3:0] s_axi_wstrb,
    input wire s_axi_wvalid,
    output wire s_axi_wready,
    output wire [1:0] s_axi_bresp,
    output wire s_axi_bvalid,
    input wire s_axi_bready,
    input wire [31:0] s_axi_araddr,
    input wire s_axi_arvalid,
    output wire s_axi_arready,
    output wire [31:0] s_axi_rdata,
    output wire [1:0] s_axi_rresp,
    output wire s_axi_rvalid,
    input wire s_axi_rready,
    
    // AXI4-Stream Output Interface
    output wire [31:0] m_axis_tdata,
    output wire m_axis_tvalid,
    input wire m_axis_tready,
    output wire m_axis_tlast,
    
    // Radar RF Interface
    output wire tx_pulse,
    input wire [ADC_WIDTH-1:0] rx_data,
    input wire rx_valid,
    
    // Interrupts
    output wire target_detected_irq,
    output wire processing_complete_irq
);

// Internal signals
wire [31:0] control_reg;
wire [31:0] status_reg;
wire [31:0] prf_reg;
wire [31:0] pulse_width_reg;
wire [31:0] range_gate_reg;
wire [31:0] doppler_bins_reg;

// DSP Chain signals
wire [ADC_WIDTH-1:0] range_processed_data;
wire range_processed_valid;
wire [ADC_WIDTH-1:0] mti_filtered_data;
wire mti_filtered_valid;
wire [ADC_WIDTH-1:0] doppler_processed_data;
wire doppler_processed_valid;
wire [15:0] detected_range;
wire [15:0] detected_velocity;
wire target_detected;

// Instantiate control registers
radar_control_regs u_control_regs (
    .clk(clk),
    .rst_n(rst_n),
    .s_axi_awaddr(s_axi_awaddr),
    .s_axi_awvalid(s_axi_awvalid),
    .s_axi_awready(s_axi_awready),
    .s_axi_wdata(s_axi_wdata),
    .s_axi_wstrb(s_axi_wstrb),
    .s_axi_wvalid(s_axi_wvalid),
    .s_axi_wready(s_axi_wready),
    .s_axi_bresp(s_axi_bresp),
    .s_axi_bvalid(s_axi_bvalid),
    .s_axi_bready(s_axi_bready),
    .s_axi_araddr(s_axi_araddr),
    .s_axi_arvalid(s_axi_arvalid),
    .s_axi_arready(s_axi_arready),
    .s_axi_rdata(s_axi_rdata),
    .s_axi_rresp(s_axi_rresp),
    .s_axi_rvalid(s_axi_rvalid),
    .s_axi_rready(s_axi_rready),
    .control_reg(control_reg),
    .status_reg(status_reg),
    .prf_reg(prf_reg),
    .pulse_width_reg(pulse_width_reg),
    .range_gate_reg(range_gate_reg),
    .doppler_bins_reg(doppler_bins_reg),
    .detected_range(detected_range),
    .detected_velocity(detected_velocity),
    .target_detected(target_detected)
);

// Instantiate pulse generator
pulse_generator u_pulse_gen (
    .clk(clk),
    .rst_n(rst_n),
    .enable(control_reg[0]),
    .prf_setting(prf_reg),
    .pulse_width_setting(pulse_width_reg),
    .tx_pulse(tx_pulse)
);

// Instantiate range processing (FFT-based matched filter)
range_processor #(
    .ADC_WIDTH(ADC_WIDTH),
    .FFT_SIZE(FFT_SIZE)
) u_range_proc (
    .clk(clk),
    .rst_n(rst_n),
    .enable(control_reg[1]),
    .rx_data(rx_data),
    .rx_valid(rx_valid),
    .range_gates(range_gate_reg),
    .processed_data(range_processed_data),
    .processed_valid(range_processed_valid)
);

// Instantiate MTI filter (2-pulse canceller)
mti_filter #(
    .DATA_WIDTH(ADC_WIDTH)
) u_mti_filter (
    .clk(clk),
    .rst_n(rst_n),
    .enable(control_reg[2]),
    .data_in(range_processed_data),
    .data_valid(range_processed_valid),
    .data_out(mti_filtered_data),
    .data_out_valid(mti_filtered_valid)
);

// Instantiate Doppler processor
doppler_processor #(
    .DATA_WIDTH(ADC_WIDTH),
    .DOPPLER_SIZE(DOPPLER_SIZE)
) u_doppler_proc (
    .clk(clk),
    .rst_n(rst_n),
    .enable(control_reg[3]),
    .data_in(mti_filtered_data),
    .data_valid(mti_filtered_valid),
    .doppler_bins(doppler_bins_reg),
    .processed_data(doppler_processed_data),
    .processed_valid(doppler_processed_valid)
);

// Instantiate CFAR detector
cfar_detector #(
    .DATA_WIDTH(ADC_WIDTH)
) u_cfar_detector (
    .clk(clk),
    .rst_n(rst_n),
    .enable(control_reg[4]),
    .data_in(doppler_processed_data),
    .data_valid(doppler_processed_valid),
    .threshold_scale(control_reg[31:16]),
    .detected_range(detected_range),
    .detected_velocity(detected_velocity),
    .target_detected(target_detected)
);

// Output data formatting
assign m_axis_tdata = {detected_velocity, detected_range};
assign m_axis_tvalid = target_detected;
assign m_axis_tlast = target_detected;

// Interrupt generation
assign target_detected_irq = target_detected;
assign processing_complete_irq = doppler_processed_valid;

endmodule
