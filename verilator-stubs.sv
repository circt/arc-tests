module plusarg_reader #(
  parameter FORMAT,
  parameter WIDTH,
  parameter [WIDTH-1:0] DEFAULT
)(
   output [WIDTH-1:0] out
);
  assign out = '0;
endmodule
