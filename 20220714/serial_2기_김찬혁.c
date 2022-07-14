/*******************************************************************************

 *  Functions

 *******************************************************************************

 */

/*******************************************************************************

 *  INCLUDE #define POLYNORMIAL 0xA001FILES

 *******************************************************************************

 */
#include <stdio.h>
#include <stdlib.h>
//multi thread
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <termios.h>                   // B115200, CS8 등 상수 정의
#include <fcntl.h>                     // O_RDWR , O_NOCTTY 등의 상수 정의
#include <time.h>
#include <math.h>
#include <errno.h> // Error integer and strerror() function
#include <unistd.h> // write(), read(), close()

/*******************************************************************************

 *  Defines

 *******************************************************************************

 */
#define _USE_MATH_DEFINES
typedef unsigned char BYTE;
#define DEG2RAD(x) (M_PI/180.0*(x) )
#define RAD2DEG(x) ((x)*180.0/M_PI)
#define RPM2RPS(x) ((x)/60)
#define RPS2RPM(x) ((x)*60)

union
{
    short data ;
    char  bytedata[2];
} m_crc;
#define DEG2RAD(x) (M_PI/180.0*(x) )
#define RAD2DEG(x) ((x)*180.0/M_PI)
#define RPM2RPS(x) ((x)/60)
#define RPS2RPM(x) ((x)*60)
#define BAUDRATE   B38400
//#define SERIAL_DEVICE   "/dev/ttyUSB0"
#define SERIAL_DEVICE   "/dev/ttyACM0"

#define MAX_R_ANGLE -30
#define MAX_L_ANGLE 30

#define MAX_ROBOT_SPEED 60
#define MIN_ROBOT_SPEED -60

#define UDR0 0x0C
#define UCSR0A 0x00

int robot_speed = 0;
int steer_angle = 0;

 
static int uart_fd;
unsigned char protocal_test[12] ={0,};
unsigned char read_buf [20] ={0,};
void write_serial(unsigned char *buf, int len)
{
	write(uart_fd, &buf[0], len);
}

int init_serial_port(void)

{
  int serial_port = open("/dev/ttyUSB0", O_RDWR);
  // Create new termios struct, we call it 'tty' for convention
  struct termios tty;
  // Read in existing settings, and handle any error
  if(tcgetattr(serial_port, &tty) != 0) {
      printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
      return 1;
  }
  tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
  tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
  tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
  tty.c_cflag |= CS8; // 8 bits per byte (most common)
  tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
  tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO; // Disable echo
  tty.c_lflag &= ~ECHOE; // Disable erasure
  tty.c_lflag &= ~ECHONL; // Disable new-line echo
  tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
  // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
  // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)
  tty.c_cc[VTIME] = 100;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
  tty.c_cc[VMIN] = 0;
  // Set in/out baud rate to be 9600
  cfsetispeed(&tty, B115200);
  cfsetospeed(&tty, B115200);
  // Save tty settings, also checking for error
  if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
      printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
      return -1;
  }
  else
  {
      return serial_port;
  }
}
unsigned char copy_readbuf[9]={0,};
unsigned char crc_copy[2] = {0,};
int crc_checksum(void)
{
	int crc_sumdata = 0;
	for(int i = 1; i < 6; i++)
	{
		crc_sumdata += copy_readbuf[i];
	}
	return crc_sumdata;
}
void *readserial_thread(void *pt)
{
    int num_bytes = -1;
    unsigned char insert_buf;
    unsigned char read_serial_data;
    unsigned char read_serial_data_old;
    int g_ucCmdATCount = 0;
    while(1)
    {
     	while( (num_bytes = read(uart_fd, &insert_buf, 1)   ) > 0 )	
        {
			read_serial_data = insert_buf;
			//printf("%x\n",read_serial_data);
			if(read_serial_data == '#' )         // 첫번째 문자 입력..
            {
				// start_byte = '#';
				// g_ucCmdATCount = 0 ;
				read_buf[g_ucCmdATCount] = read_serial_data ;
				g_ucCmdATCount++ ;
				//printf("# Received\n");

            }   
            
			else if( (read_serial_data == 'I' ) || (read_serial_data == 'F' ))
			{
				if(read_serial_data_old =='#')
				{                      
					read_buf[0] = '#' ;
					g_ucCmdATCount = 1 ;
					read_buf[g_ucCmdATCount] = read_serial_data ;
					g_ucCmdATCount++ ;
					/*if(read_serial_data == 'F')
					//printf("F Received\n");
					else
					//printf("I Received \n");*/
				}				                  
			}
			
			else if(g_ucCmdATCount >= 2 && g_ucCmdATCount <= 5){
					read_buf[g_ucCmdATCount] = read_serial_data ;
					g_ucCmdATCount++ ;
					//printf("Byte data Received\n");
			}
			
			else if(g_ucCmdATCount >= 6 && g_ucCmdATCount <= 7){
			read_buf[g_ucCmdATCount] = read_serial_data ;

					g_ucCmdATCount++ ;

					//printf("crc data Received\n");
			}
			
			else if( read_serial_data == '*' ) // 끝문자 검색
			{
				read_buf[g_ucCmdATCount] = read_serial_data ;
				g_ucCmdATCount++ ;
				if( (read_buf[0] == '#' ) && (read_buf[1] == 'I' ))          // 첫번째 문자 검색, data accepted.. '#' + 'S' + 2byte+ 4 byte + '*'
				{
					memcpy(copy_readbuf, read_buf, g_ucCmdATCount) ;
					
					//read_buf[g_ucCmdATCount] = read_serial_data ;
					printf("Received Data:%x %x %x %x %x %x %x %x %x\t", copy_readbuf[0],copy_readbuf[1],copy_readbuf[2],copy_readbuf[3],copy_readbuf[4],read_buf[5],copy_readbuf[6],copy_readbuf[7],copy_readbuf[8]);
					m_crc.data = crc_checksum();
					//printf("%x		%x \n",m_crc.bytedata[1],m_crc.bytedata[0]);
					if(m_crc.bytedata[1] == copy_readbuf[6] && m_crc.bytedata[0] == copy_readbuf[7])
						printf("NORMAL \n");
					else
						printf("ERROR \n");
				}
				g_ucCmdATCount = 0 ;
			}                  
			else
			{
				read_buf[g_ucCmdATCount] = read_serial_data ;
				g_ucCmdATCount++ ;
			}

			read_serial_data_old =  read_serial_data;
		}
     }
    }


int main(void)

{

  uart_fd = init_serial_port();

  //memset(&read_buf, '\0', sizeof(read_buf));

  //unsigned char msg[] = { 'H', 'e', 'l', 'l', 'o', '\r' };

  //write(uart_fd , msg, sizeof(msg));

  // Normally you wouldn't do this memset() call, but since we will just receive

  // ASCII data for this example, we'll set everything to 0 so we can

  // call printf() easily.

  // Read bytes. The behaviour of read() (e.g. does it block?,

  // how long does it block for?) depends on the configuration

  // settings above, specifically VMIN and VTIME

  pthread_t id_1;

  int ret1=pthread_create(&id_1,NULL,readserial_thread,NULL);

  // Here we assume we received ASCII data, but you might be sending raw bytes (in that case, don't try and

  // print it to the screen like this!)

  while(1)

  {

      //m_robot_angle.data--;

      //m_robot_speed.data -=0.01;

      //send_serial_data();

      //printf("%x %x  %x %x  %x %x\n",m_robot_angle.bytedata[0],m_robot_angle.bytedata[1], m_robot_speed.bytedata[0], m_robot_speed.bytedata[1],m_robot_speed.bytedata[2], m_robot_speed.bytedata[3] );
      //printf("Received Data: %x %x %x %x %x %x %x %x %x %x\n", read_buf[0],read_buf[1],read_buf[2],read_buf[3],read_buf[4],read_buf[5],read_buf[6],read_buf[7],read_buf[8],read_buf[9]);

      sleep(1);

  }

  close(uart_fd);

  return 0;

}
