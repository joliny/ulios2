OUTPUT_FORMAT("binary")
ENTRY(_start)
SECTIONS
{
	gdt =	0;
	idt =	0x0800;
	kpdt =	0x1000;
	pddt =	0x2000;
	pddt0 =	0x3000;
	kmmt =	0x4000;
	pmt =	0x7000;
	exmt =	0x8000;
	kpt =	0x9000;

	KnlValue =	0xF000;

	.text 0x10000: {*(.text)}
	.data : {*(.data)}
	.bss : {*(.bss)}

	BaseSrv =	0x90080;
	VesaMode =	0x900FC;
	VbeInfo =	0x90100;
	ModeInfo =	0x90300;
	HdInfo =	0x90400;
	MemEnd =	0x90420;
	ards =		0x90424;

	gramem =	0x000A0000;
	txtmem =	0x000B8000;
	kdat =		0x00100000;
	pdt =		0x00800000;
	pdt0 =		0x00C00000;
	pt =		0x01000000;
	pt0 =		0x01400000;
	pt2 =		0x01800000;
	pg0 =		0x01C00000;
}
