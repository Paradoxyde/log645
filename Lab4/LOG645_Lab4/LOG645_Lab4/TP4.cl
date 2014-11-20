__kernel void HeatTransfer(__global int* buf, int index)
{
	buf[index] += index + 1;
}
