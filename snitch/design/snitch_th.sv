// Test harness for the Snitch core to simplify interaction with the model.
module snitch_th (
  input  logic        clk_i,
  input  logic        rst_i,

  output logic [31:0] inst_addr_o,
  input  logic [31:0] inst_data_i,
  output logic        inst_valid_o,
  input  logic        inst_ready_i,

  output logic        mem_valid_o,
  output logic [31:0] mem_addr_o,
  output logic        mem_write_o,
  output logic [63:0] mem_wdata_o,
  output logic [7:0]  mem_wstrb_o,
  input  logic        mem_ready_i,
  input  logic [63:0] mem_rdata_i
);
  logic [31:0]  acc_qaddr_o;
  logic [4:0]   acc_qid_o;
  logic [31:0]  acc_qdata_op_o;
  logic [63:0]  acc_qdata_arga_o;
  logic [63:0]  acc_qdata_argb_o;
  logic [63:0]  acc_qdata_argc_o;
  logic         acc_qvalid_o;
  logic         acc_qready_i = '0;
  logic [63:0]  acc_pdata_i = '0;
  logic [4:0]   acc_pid_i = '0;
  logic         acc_perror_i = '0;
  logic         acc_pvalid_i = '0;
  logic         acc_pready_o;

	logic [31:0]  data_qaddr_o;
	logic         data_qwrite_o;
	logic [3:0]   data_qamo_o;
	logic [63:0]  data_qdata_o;
	logic [7:0]   data_qstrb_o;
	logic         data_qvalid_o;
	logic         data_qready_i;
	logic [63:0]  data_pdata_i;
	logic         data_perror_i;
	logic         data_pvalid_i;
	logic         data_pready_o;

	snitch #(
		.BootAddr(32'h0001_0000)
	) i_snitch (
		.clk_i,
		.rst_i,
		.hart_id_i(32'hd0),

		.inst_addr_o,
		.inst_data_i,
		.inst_valid_o,
		.inst_ready_i,

    .acc_qaddr_o,
    .acc_qid_o,
    .acc_qdata_op_o,
    .acc_qdata_arga_o,
    .acc_qdata_argb_o,
    .acc_qdata_argc_o,
    .acc_qvalid_o,
    .acc_qready_i,
    .acc_pdata_i,
    .acc_pid_i,
    .acc_perror_i,
    .acc_pvalid_i,
    .acc_pready_o,

		.data_qaddr_o,
		.data_qwrite_o,
		.data_qamo_o,
		.data_qdata_o,
		.data_qstrb_o,
		.data_qvalid_o,
		.data_qready_i,
		.data_pdata_i,
		.data_perror_i,
		.data_pvalid_i,
		.data_pready_o,

    .wake_up_sync_i(1'b0),
    .fpu_rnd_mode_o(),
    .fpu_status_i('0),
    .core_events_o()
	);

  // logic buffer_store;

  // always_comb begin
  //   buffer_store = 0;
  //   mem_addr_o = 0;
  //   mem_valid_o = 0;
  //   mem_write_o = 0;
  //   mem_wstrb_o = 0;
  //   mem_wdata_o = 0;
  //   inst_data_i = 0;
  //   inst_ready_i = 0;
  //   data_qready_i = 0;

  //   if (data_qvalid_o) begin
  //     if (!data_pvalid_i) begin
  //       mem_valid_o = 1;
  //       mem_addr_o = data_qaddr_o;
  //       mem_wdata_o = data_qdata_o;
  //       mem_wstrb_o = data_qstrb_o;
  //       data_qready_i = mem_ready_i;
  //       buffer_store = mem_ready_i;
  //     end
  //   end else if (inst_valid_o) begin
  //     mem_valid_o = 1;
  //     mem_addr_o = inst_addr_o;
  //     inst_data_i = mem_rdata_i;
  //     inst_ready_i = mem_ready_i;
  //   end
  // end

  // logic pick_data = 0;

  // always_ff @(posedge clk_i) begin
  //   if (rst_i)
  //     pick_data <= 0;
  //   else if (mem_ready_i)
  //     pick_data <= ~pick_data;
  // end

  // assign mem_valid_o = pick_data ? data_qvalid_o : inst_valid_o;
  // assign mem_addr_o = pick_data ? data_qaddr_o : inst_addr_o;
  // assign data_qready_i = pick_data ? mem_ready_i : 0;
  // assign inst_ready_i = pick_data ? 0 : mem_ready_i;

  // always_ff @(posedge clk_i) if (rst_i) begin
  //   state <= Idle;
  // end else if (state == Idle) begin
  //   if (inst_valid_o)
  //     state <= Inst;
  //   if (data_qvalid_o)
  //     state <= Data;
  // end else if (state == Inst) begin
  // end else if (state == Data) begin
  // end

  // always_comb begin
  //   mem_addr_o = 0;
  //   if (state == Inst) begin
  //     mem_addr_o = inst_addr_o;
  //   end else if (state == Data) begin
  //     mem_addr_o = data_qaddr_o;
  //   end
  // end

  // assign mem_valid_o = state != Idle;

  // always_comb begin
  //   inst_ready_i = 0;
  //   inst_data_i = 0;
  //   if (state == Inst) begin
  //     inst_ready_i = mem_ready_i;
  //     inst_data_i = mem_rdata_i;
  //   end
  // end

  // always_comb begin
  //   mem_valid_o = 0;
  //   mem_addr_o = 0;
  //   which = 0;
  //   if (data_qvalid_o) begin
  //     mem_valid_o = 1;
  //     mem_addr_o = data_qaddr_o;
  //     which = 0;
  //   end else if (inst_valid_o) begin
  //     mem_valid_o = 1;
  //     mem_addr_o = inst_addr_o;
  //     which = 1;
  //   end
  // end

  // always_comb begin
  //   inst_ready_i = 0;
  //   data_qready_i = 0;
  //   if (which) begin
  //     inst_ready_i = mem_ready_i;
  //   end else begin
  //     data_qready_i = mem_ready_i;
  //   end
  // end

  // always_ff @(posedge clk_i) begin
  //   if (rst_i) begin
  //     state <= Idle;
  //   end else begin
  //     state <= state_next;
  //   end
  // end

endmodule
