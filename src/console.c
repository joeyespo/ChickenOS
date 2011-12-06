#include <kernel/memory.h>
#include <kernel/console.h>
#include <kernel/hw.h>
#include <../fs/vfs.h>
static  uint16_t * videoram = (uint16_t *) 0xc00b8000;
#define BLANK 0x0700
/* some stuff that will later be used to setup mutliple consoles */
#define NUM_CONSOLES 3
#define CURRENT_CONSOLE consoles[current_console]
typedef struct {
	uint16_t *videoram;
	uint16_t *buffer;
	uint8_t x,y;
	uint16_t attribute;
	int num;
} console_t;
console_t console0;
console_t console1;
console_t console2;
console_t *consoles[NUM_CONSOLES] = {&console0, &console1, &console2};
console_t *console = &console0;

#define CURSOR_POS (console->x + console->y*80)

static void console_cursor_move(uint16_t pos)
{
	outb(0x3D4, 14); //cursor highbyte register selected
	outb(0x3D5, pos >> 8);
	outb(0x3D4, 15); //cursor lowbyte register selected
	outb(0x3D5, pos & 0xFF);
}
void console_set_color(uint8_t bg_color, uint8_t fg_color)
{
	console->attribute = (bg_color << 12) | (fg_color << 8) ;
}

/* rewrite this later with memcpy and memset */
static void console_scroll(console_t *con)
{
	/* move lines up one */
	for(int i = 0; i < 24; i++)
	{
		kmemcpyw(&con->videoram[i*80],&con->videoram[(i + 1)*80], 80);
	}
	/* blank last line */
	kmemsetw(&con->videoram[24*80], BLANK, 80);

	con->y = 24;
}



void tty_putc(console_t *con, uint8_t c)
{
	con->videoram = videoram;
	switch(c)
	{
		case 0x08:
			if(con->x != 0)
				con->x--;
			break;
		//case 0x29://tab
		case '\t'://tab
			con->x = (con->x + 8) & ~(7);
			break;
		case '\n':
			con->y++;
		case '\r':
			con->x = 0;
			break;
		default:
			con->videoram[CURSOR_POS] = con->attribute | c;
			con->x++;
	}
	if(console->x >= 80) 
	{
		con->x = 0;
		con->y++;
	}
	if(con->y >= 25)
	{
		console_scroll(con);
	//	console.y = 0;
	}
	if(con == console)
	console_cursor_move(CURSOR_POS);
}

void console_putc(uint8_t c)
{
	tty_putc(console, c);
}

int console_puts(char *string)
{
	int cnt = 0;

	while(*string != '\0')
	{
		console_putc(*string++);
		cnt++;
	}

	return cnt;
}

void console_clear()
{
	kmemsetw(console->videoram, BLANK, 80*25);
	console->y=console->x = 0;
	console_cursor_move(0);
}
#include <kernel/vm.h>
void console_init()
{
	console->x = console->y = 0;
	console->attribute = BLANK;
	console->videoram = videoram;
//	console->buffer = palloc();
	console->num = 0;
//	console1.buffer = palloc();
	console1.attribute = BLANK;
	console1.x=console1.y = 0;
	console1.num = 1;
	console_clear();
	kbd_init();
}

char console_getc()
{
	
	return '\n';
	return kbd_getc();


}

char tty_getc(console_t *con)
{
	while(console != con);
			
	return kbd_getc();

}

void console_switch(int num)
{
	//copy curent vram to buffer
	kmemcpyw(console->buffer,console->videoram, 80*25*2);
	console->videoram = console->buffer;	
	//copy new consoles buffer to vram
	console = consoles[num];
	console->videoram = videoram;
	kmemcpyw(console->videoram, console->buffer, 80*25*2);	
	num = num;	
//	console = consoled[num];

	//put cursor at correct postion
	console_cursor_move(CURSOR_POS);
}

char *derp = "DERPDERPDERPDERP";

size_t console_read(uint16_t dev, void *_buf, off_t off, size_t count)
{
	char *buf = _buf;
	off = off;
	size_t read = count;
	dev = dev;
	int tty = MINOR(dev);
	while(count--)
	{
		*buf++ = tty_getc(consoles[tty]);
	}
	return read;
}

size_t console_write(uint16_t dev, void *_buf, off_t off, size_t count)
{
	char *buf = _buf;
	dev = dev;
	off = off;
	size_t written = count;
	int tty = MINOR(dev);
	while(count--)
	{
		tty_putc(consoles[tty],*buf++);
	}

	return written;
}

void console_fs_init()
{
	struct file * dir = pathsearch(root, "/dev");
	struct file *new = file_new3("tty", FILE_CHAR, 0x800);
	insert_file(dir, new);

	device_file_register(0x800, console_read, console_write);
	new = file_new3("tty0", FILE_CHAR, 0x801);
	insert_file(dir, new);

	device_file_register(0x800, console_read, console_write);

}

