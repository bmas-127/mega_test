#include "mbed.h"
#include "bbcar.h"
#include "mbed_rpc.h"
#include "fsl_port.h"
#include "fsl_gpio.h"

// servo motor
DigitalOut led1(LED1);
PwmOut pin9(D9), pin8(D8);
DigitalInOut pin10(D10);
Ticker servo_ticker;
BBCar car(pin8, pin9, servo_ticker);

// encoder
DigitalIn Din_11(D13);
Ticker encoder_ticker;

// ping
parallax_ping  ping1(pin10);
parallax_encoder encoder( Din_11, encoder_ticker );

// XBee and Xbee rpc
//void getLog(Arguments *in, Reply *out);
void background_Acc();
RawSerial pc(USBTX, USBRX);
RawSerial xbee(D12, D11);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
//RPCFunction rpcLog(&getLog, "get");
Thread t;

// state
int state = 0, mission = -1;

// log thread
void sendlog();
Thread thread_sendlog;
Timer timer;


void GoStraight(int speed_left, int speed_right, int dist, bool direction);
void TurnDirection(int speed_left, int speed_right, int step);
void classification();

void xbee_setting();
void xbee_rx_interrupt(void);
void xbee_rx(void);
void reply_messange(char *xbee_reply, char *messange);
void check_addr(char *xbee_reply, char *messenger);

int main() {
    pc.baud(9600);
    pc.printf("suck  ===========  dick");
    xbee_setting();


    thread_sendlog.start(sendlog);

    car.stop();
/*
    // go straight first road
    GoStraight(100, 105, 20, 1);

    // turn left
    TurnDirection(-50, 50, 12);

    // go straight to parking lot
    mission = 1;
    GoStraight(100, 105, 10, 1);

    // parking
    TurnDirection(50, -50, 12);
    GoStraight(-100, -107, 40, 0);

    // leaving parking lot and going to photo point
    GoStraight(100, 107, 25, 1);
    TurnDirection(50, -50, 12);
    car.goStraight(100, 102);
    wait(1);
    TurnDirection(-50, 50, 12);

    // take a photo
    state = 4;
    wait(10);
*/
    // go ahead to mission 2
    mission = 0;
   // TurnDirection(50, -50, 12);
    GoStraight(100, 105, 40, 1);
    TurnDirection(50, -50, 12);

    // go to ultra point
    GoStraight(100, 105, 20, 1);
    TurnDirection(50, -50, 12);
    car.goStraight(100, 107);
    wait(2);
    TurnDirection(50, -50, 12);
    car.goStraight(100, 105);
    wait(2);
    TurnDirection(-50, 50, 12);

    // classification
    classification();

    // back to endpoint
    mission = 3;
    TurnDirection(-50, 50, 20);
    GoStraight(100, 105, 20, 1);
    TurnDirection(50, -50, 12);
    GoStraight(100, 105, 20, 1);
    TurnDirection(50, -50, 12);
    GoStraight(100, 105, 20, 1);

    
 
}

void classification()
{
    state = 2;
    float ping_dis[4];

    car.goStraight(50, -50);
    wait(0.38);
    car.stop();
    wait(1);
    car.goStraight(-50, 50);

    for(int i = 0; i < 4; i ++){
      ping_dis[i] = float(ping1);
      wait(0.25);
    }
    
    car.stop();

    if(ping_dis[0] > ping_dis[1] && ping_dis[2] < ping_dis[3]) { //triangle
      xbee.printf("Triangle!!\r\n");
    }else if(ping_dis[0] == ping_dis[1] && ping_dis[1] == ping_dis[2] && ping_dis[2] == ping_dis[3]) { //rectangle
      xbee.printf("Rectangle!!\r\n");
    }else if(ping_dis[0] > ping_dis[1] && ping_dis[1] > ping_dis[2] && ping_dis[2] > ping_dis[3]) { //right triangle
      xbee.printf("Right Triangle!!\r\n");
    }else {
      xbee.printf("Tooth Pattern!!\r\n");
    }

    wait(2);
}


void GoStraight(int speed_left, int speed_right, int dist, bool direction){
    state = 0;
    int cnt = 0;

    car.stop();
    wait(0.5);

    if(direction)
        car.goStraight(100, 255);
    else
        car.goStraight(-100, -255);
    
    
    wait(0.05);
    car.goStraight(speed_left, speed_right);

    while(true){
        if(direction){
            //xbee.printf("%f\r\n", (float)ping1 );
            if((float)ping1 < dist){
                led1 = 0;
                cnt ++;
            }
            else {
                led1 = 1;
            }
        }else{
            state = 3;
            wait(3);
            cnt = 2;
            car.stop();
        }

        if(cnt == 2) break;

        wait(0.1);
    }
}

void TurnDirection(int speed_left, int speed_right, int num){
    state = 1;

    car.stop();
    wait(0.5);

    car.goStraight(speed_left, speed_right);
    
    encoder.reset();
    while(true){
        int step = encoder.get_steps();
        if(step >= num) break;
    }

    car.stop();
    wait(1);
}

void background_Acc(){
  for(int i = 0; i < 30; i ++){
    pc.printf("suck my dick\r\n");
    xbee.printf("suck my dick\n\n");
    wait(1);
  }
}

void xbee_setting(){
    char xbee_reply[4];

    // XBee setting
    xbee.baud(9600);
    xbee.printf("+++");
    xbee_reply[0] = xbee.getc();
    xbee_reply[1] = xbee.getc();
    if(xbee_reply[0] == 'O' && xbee_reply[1] == 'K'){
        pc.printf("enter AT mode.\r\n");
        xbee_reply[0] = '\0';
        xbee_reply[1] = '\0';
    }
    xbee.printf("ATMY 0x209\r\n");
    reply_messange(xbee_reply, "setting MY : 0x209");

    xbee.printf("ATDL 0x109\r\n");
    reply_messange(xbee_reply, "setting DL : 0x109");

    xbee.printf("ATID 0x1\r\n");
    reply_messange(xbee_reply, "setting PAN ID : 0x1");

    xbee.printf("ATWR\r\n");
    reply_messange(xbee_reply, "write config");

    xbee.printf("ATMY\r\n");
    check_addr(xbee_reply, "MY");

    xbee.printf("ATDL\r\n");
    check_addr(xbee_reply, "DL");

    xbee.printf("ATCN\r\n");
    reply_messange(xbee_reply, "exit AT mode");
    xbee.getc();

    // start

  pc.printf("start\r\n");

  t.start(callback(&queue, &EventQueue::dispatch_forever));


  // Setup a serial interrupt function to receive data from xbee

  xbee.attach(xbee_rx_interrupt, Serial::RxIrq);
}

void sendlog(){   
  while(true){
    if(state == 0){
      if(mission == -1) xbee.printf("go foward to mission 1\r\n");
      else if (mission == 0) xbee.printf("go foward to mission 2\r\n");
      else if (mission == 3) xbee.printf("go foward to end point\r\n");
      else xbee.printf("forwarding, executioning mission %d\r\n", mission);
    }        
    else if(state == 1)
      xbee.printf("turn conner\r\n");          
    else if(state == 2)
      xbee.printf("scannig obecject and classifying\r\n");     
    else if(state == 3)
      xbee.printf("go backward\r\n");               
    else if(state == 4)
      xbee.printf("take a photo\r\n");    

    wait(1);
  }
}

void xbee_rx_interrupt(void)

{

  xbee.attach(NULL, Serial::RxIrq); // detach interrupt

  queue.call(&xbee_rx);

}


void xbee_rx(void)

{

  char buf[100] = {0};

  char outbuf[100] = {0};

  while(xbee.readable()){

    for (int i=0; ; i++) {

      char recv = xbee.getc();

      if (recv == '\r') {

        break;

      }

      buf[i] = pc.putc(recv);

    }

    RPC::call(buf, outbuf);

    pc.printf("%s\r\n", outbuf);

    wait(0.1);

  }

  xbee.attach(xbee_rx_interrupt, Serial::RxIrq); // reattach interrupt

}

void reply_messange(char *xbee_reply, char *messange){

  xbee_reply[0] = xbee.getc();

  xbee_reply[1] = xbee.getc();

  xbee_reply[2] = xbee.getc();

  if(xbee_reply[1] == 'O' && xbee_reply[2] == 'K'){

   pc.printf("%s\r\n", messange);

   xbee_reply[0] = '\0';

   xbee_reply[1] = '\0';

   xbee_reply[2] = '\0';

  }

}


void check_addr(char *xbee_reply, char *messenger){

  xbee_reply[0] = xbee.getc();

  xbee_reply[1] = xbee.getc();

  xbee_reply[2] = xbee.getc();

  xbee_reply[3] = xbee.getc();

  pc.printf("%s = %c%c%c\r\n", messenger, xbee_reply[1], xbee_reply[2], xbee_reply[3]);

  xbee_reply[0] = '\0';

  xbee_reply[1] = '\0';

  xbee_reply[2] = '\0';

  xbee_reply[3] = '\0';

}