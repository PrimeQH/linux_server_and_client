#include "utility.h"
//#include<vector>
#include <map>
#include <sys/types.h>//mkfifo
#include <sys/stat.h>//mkfifo
#include <limits.h> //PIPE_BUF定义


#define ratio_hdf 11
#define ratio_bss 3


const int CLIENTNUM=2301;//从1开始
//vector<int> sockets(CLIENTNUM);
const int SLEEP_US=10000;
const int CLIENTSEC=500;
const int MINCLIENTSEC=50;
const int LIVETIME=300;

map<int,int> map_ID_sockets;//从1开始
map<int,CLIENT> map_socket_clients;



//hash函数生成密钥
unsigned int JSHash(char* str)
{
    unsigned int len = strlen(str);
    unsigned int hash = 1315423911;
    unsigned int i    = 0;

    for(i = 0; i < len; i++)
    {
        hash ^= ((hash << 5) + str[i] + (hash >> 2));
    }

    return hash;
}

//possion(泊松)分布（possion distribution）
int randomPossion(double lambda)   //原返回值是int
{
    int x = -1;
    double log1, log2 , u;
    log1 = 0;
    log2 = -lambda;
    do
    {
        u = rand()%1000/1000.0;
        //printf("u=%lf\n",u);
        log1 += log(u);
        x++;
    }
    while(log1 >= log2);
    return x;
}



//负指数函数
double expntl(double x)
{
    double z;
    do
    {
        //	 cout<<rand()<<endl;
        z = ((double) rand() / RAND_MAX);
    }
    while ((z == 0) || (z == 1));
    return(-x * log(z)); //z相当于1-x,而x相当于1/lamda。
}

void infoprint(int id, int hdf, int bss,double life_time)
{
    if(0==hdf)
{
if(0==bss)
        printf("USER ID : %d , handover call , Voice service , lifetime:%lf, connecting....\n",id,life_time);
    else if(1==bss)
        printf("USER ID : %d , handover call , Streaming media service , lifetime:%lf, connecting....\n",id,life_time);
    else
        printf("USER ID : %d , handover call , Data service , lifetime:%lf, connecting....\n",id,life_time);
}

    else
    {
    if(0==bss)
        printf("USER ID : %d , new call , Voice service , lifetime:%lf, connecting....\n",id,life_time);
    else if(1==bss)
        printf("USER ID : %d , new call , Streaming media service, lifetime:%lf, connecting....\n",id,life_time);
    else
        printf("USER ID : %d , new call , Data service , lifetime:%lf, connecting....\n",id,life_time);
    }


}

int main()
{
    ///定义sockfd
    //int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    //int countnow=0;
    ///定义sockaddr_in
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);  ///服务器端口
    //serverAddr.sin_addr.s_addr = inet_addr("121.42.143.201");  ///服务器ip
    //serverAddr.sin_addr.s_addr = inet_addr("192.168.0.103");
    //serverAddr.sin_addr.s_addr = inet_addr("192.168.1.102");
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srand((unsigned) time(NULL)); //为了提高不重复的概率
    struct rlimit rt;//资源限制符
    //设置每个进程允许打开的最大文件数
    rt.rlim_max=rt.rlim_cur=EPOLL_SIZE;
    if(setrlimit(RLIMIT_NOFILE,&rt)==-1)
    {
        perror("setrlimt error.\n");
        return -1;
    }

    int epfd = epoll_create(EPOLL_SIZE);  //创建epoll
    if(epfd < 0)
    {
        perror("epfd error");
        return -1;
    }
    static struct epoll_event events[EPOLL_SIZE]; //epoll中事件的结构体数组，一个socket算一个事件

    //创建互不相同的随机数
    int a[CLIENTNUM],i;
    for(i=0; i<CLIENTNUM; ++i) a[i]=i+1;
    for(i=CLIENTNUM; i>=1; --i) swap(a[i], a[rand()%i]);



    // 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
    int pipe_fd[2];
    if(pipe(pipe_fd) < 0)
    {
        perror("pipe error");
        return -1;
    }

    //将管道读端描述符添加到内核事件表中
    addfd(epfd, pipe_fd[0], true);

    // Fork
    int pid = fork();
    if(pid < 0)
    {
        perror("fork error");
        return -1;
    }
    else if(pid == 0)   // 子进程
    {
        //子进程负责写入管道，因此关闭读端
        close(pipe_fd[0]);

        bool isClientwork=true;
        int iNum = 0;
        char message[BUF_SIZE];
        bzero(message, BUF_SIZE);


        int count_client=0;
        int client_sec=CLIENTSEC;

        while(isClientwork)
        {

            //cout<<"end sleep"<<endl;
            /*   printf("Please input the sockets' ID: ");

               int ID;
               cin>>ID;
               if(cin.fail())
               {
                   cout<<"input is wrong, please try again."<<endl;
                   cin.clear();
                   cin.ignore(4096,'\n');
                   continue;
               }

               //判别ID是否存在的工作交给父进程

               printf("Please input the order: ");
               // 聊天信息缓冲区
               char order[BUF_SIZE];
               bzero(order, BUF_SIZE);
               cin>>order;
               char message[BUF_SIZE];
               bzero(message, BUF_SIZE);
               if(strcmp(order,"00")==0)
               {
                   sprintf(message,"%s %d %s",order,ID," ");
               }
               else if(strcmp(order,"-1")==0)//关闭当前socket
               {
                   sprintf(message,"%s %d %s",order,ID," ");
               }
               else
               {
                   printf("Please input the message: ");
                   // 聊天信息缓冲区
                   char msg[BUF_SIZE];
                   bzero(msg, BUF_SIZE);
                   cin>>msg;
                   sprintf(message,"%s %d %s",order,ID,msg);
               }
               // 子进程将信息写入管道
               if( write(pipe_fd[1], message, strlen(message)  ) < 0 )//zsd
               {
                   perror("fork error");
                   return -1;
               }*/

            if(count_client>1000)
            {
                client_sec=CLIENTSEC-count_client/(2000/(CLIENTSEC-MINCLIENTSEC));
                if(client_sec<MINCLIENTSEC) client_sec=MINCLIENTSEC;
            }
            int possion =randomPossion(client_sec);
            count_client+=possion;
            //printf("possion=%d\n",possion);
            //int possion=2;
            for(int itemp=0; itemp<possion; itemp++)
            {
                //printf("itemp = %d\n",itemp);
                bzero(message,BUF_SIZE);
               /* if(itemp !=possion-1)
                {
                    sprintf(message,"%s %d %s","00",a[iNum]," \0");
                }
                else
                {
                    sprintf(message,"%s %d %s","00",rand()%200+2000," \0");
                    iNum--;
                }*/
                sprintf(message,"%s %d %s","00",a[iNum]," \0");
                // 子进程将信息写入管道
                if( write(pipe_fd[1], message, strlen(message)  ) < 0 )//zsd
                {
                    perror("fork error");
                    return -1;
                }
                usleep(SLEEP_US);
                if(iNum==CLIENTNUM)
                {
                    for(i=0; i<CLIENTNUM; ++i) a[i]=i+1;
                    for(i=CLIENTNUM; i>=1; --i) swap(a[i], a[rand()%i]);
                    iNum=0;
                }
                iNum++;


            }
            //printf("this possion is end. start sleep.\n");
            int s=1000*1000-possion*SLEEP_US;
            if(s<0) s=0;
            if(s>0) usleep(s);
            //sleep(1);
        }
    }
    else   //pid > 0 父进程
    {
        //父进程负责读管道数据，因此先关闭写端
        close(pipe_fd[1]);
        bool isClientwork=true;
        const char* fifo_name = "/tmp/my_fifo_cl";
        int res=0;//
        int pipecntonly=-1;
        const int open_mode = O_WRONLY | O_NONBLOCK;
        char buf_count[PIPE_BUF+1];
        if(access(fifo_name,F_OK)==-1)
        {
            //管道文件不存在
            //创建命名管道
            res=mkfifo(fifo_name,0777);
            if(res != 0)
            {
                fprintf(stderr,"cannot create fifo %s\n",fifo_name);
                exit(EXIT_FAILURE);
            }
        }
        pipecntonly=open(fifo_name,open_mode);
        if(pipecntonly==-1)
            exit(EXIT_FAILURE);
        else
        {
            while(isClientwork)
            {
                int epoll_events_count = epoll_wait( epfd, events, EPOLL_SIZE, -1 );
                //处理就绪事件
                //printf("epoll_events_count=%d\n",epoll_events_count);
                for(int i = 0; i < epoll_events_count ; ++i)
                {
                    // 聊天信息缓冲区
                    char message[BUF_SIZE];
                    bzero(message, BUF_SIZE);
                    if(events[i].data.fd == pipe_fd[0])
                    {
                        //printf("read from the pipe.\n");
                        char order[ORDER_LEN+1];
                        bzero(order, sizeof(order));
                        int ID;
                        char msg[BUF_SIZE];
                        bzero(msg, BUF_SIZE);
                        //从管道读子进程键盘输入的指令
                        int ret = read(events[i].data.fd, message, BUF_SIZE);
                        if(ret == 0)
                        {
                            //isClientwork = 0;
                        }
                        else
                        {
                            sscanf(message,"%s %d %s",order,&ID,msg);
                            //printf("msg = %s\n",msg);




                            map<int,int>::iterator map_int_int_it;
                            map_int_int_it=map_ID_sockets.find(ID);
                            if(map_int_int_it==map_ID_sockets.end() ) //没找到
                            {
                                //printf("This ID's socket can't find, you must enroll(00) it.\n");
                                // 创建socket
                                map_ID_sockets[ID]= socket(PF_INET, SOCK_STREAM, 0);
                                // 连接服务端
                                if(connect(map_ID_sockets[ID], (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
                                {
                                    perror("connect error");
                                    return -1;
                                }
                                //将sock添加到内核事件表中
                                addfd(epfd, map_ID_sockets[ID], true);
                            }

                            if(strcmp(order,"00")==0)
                            {
                                char send_message[BUF_SIZE];
                                bzero(send_message, BUF_SIZE);
                                int n;
                                struct CLIENT sendUser;
                                bzero(&sendUser,sizeof(sendUser));

                                sendUser.id=ID;
                                //n=520520;
                                sendUser.pwd=GenKey(ID);
                                n=rand()%ratio_hdf;
                                if(n!=0)
                                    n=1;
                                //	printf("n=%d\n",n);
                                sendUser.hdf_type=n;
                                //if(n==0)
                                  //  sendUser.bss_type=n;
                                //else
                                sendUser.bss_type=rand()%ratio_bss;
                                sendUser.life_time=expntl(LIVETIME);
                                sendUser.degree=-1;
                                sendUser.sockfd=-1;
                                sendUser.state=0;
                                map_socket_clients[map_ID_sockets[ID]]=sendUser;
                                // 将信息发送给服务端
                                strcat(send_message,order);
                                //strcat(&send_message[ORDER_LEN],client_info);//zsd//只能压进第一个，但服务端就可以全部收到
                                memcpy(&send_message[strlen(order)],&sendUser,sizeof(CLIENT));
                                //send_message[sizeof(CLIENT)+ORDER_LEN]='\0';
                                //strcat(&send_message[ORDER_LEN],sendUser.usr_pwd);
                                send(map_ID_sockets[ID],send_message, sizeof(CLIENT)+ORDER_LEN, 0);

                            }
                            else if(strcmp(order,"-1")==0)//关闭当前socket
                            {
                                map<int,CLIENT>::iterator map_int_client_it;
                                map_int_client_it=map_socket_clients.find(map_ID_sockets[ID]);
                                char send_message[BUF_SIZE];
                                bzero(send_message, BUF_SIZE);
                                strcat(send_message,order);
                                strcat(&send_message[ORDER_LEN],msg);
                                //send_message[]='\0';
                                send(map_ID_sockets[ID],send_message, strlen(send_message), 0);
                                printf("send message2: %s\n",send_message);
                                close(map_ID_sockets[ID]);
                                delfd(epfd, map_ID_sockets[ID], true);/////////////////////
                                printf("ClientID = %d closed.\n", ID);//zsd
                                map_ID_sockets.erase(map_int_int_it);
                                map_socket_clients.erase(map_int_client_it);
                                //printf("client count now = %lu\n\n\n",map_socket_clients.size());
                                bzero(buf_count,sizeof(buf_count));
                                //sprintf(buf_count,"client count now = %lu\n",map_socket_clients.size());
                                sprintf(buf_count,"Poisson arrival rate = %d\nclient count now = %lu\n",MINCLIENTSEC,map_socket_clients.size());
                                res=write(pipecntonly,buf_count,sizeof(buf_count));
                                if(res==-1)
                                {
                                    fprintf(stderr, "Write error on pipe\n");
                                    //exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                char send_message[BUF_SIZE];
                                bzero(send_message, BUF_SIZE);
                                strcat(send_message,order);
                                strcat(&send_message[ORDER_LEN],msg);
                                //send_message[sizeof(CLIENT)+ORDER_LEN]='\0';
                                send(map_ID_sockets[ID],send_message, strlen(send_message), 0);
                                printf("send message3: %s\n",send_message);
                            }

                        }
                    }
                    else//从服务器接收的
                    {
                        //printf("recv from the server.\n");
                        //服务端发来消息
                        int sock=events[i].data.fd;
                        //接受服务端消息
                        int ret = recv(sock, message, BUF_SIZE, 0);

                        // ret= 0 服务端将该socket关闭
                        if(ret <= 0)
                        {
                            map<int,int>::iterator map_int_int_it;
                            for(map_int_int_it=map_ID_sockets.begin(); map_int_int_it!=map_ID_sockets.end(); ++map_int_int_it)
                            {
                                if(map_int_int_it->second==sock) break;
                            }
                            if(map_int_int_it==map_ID_sockets.end()) printf("Can't find the socket to close.\n");
                            else
                            {
                                //printf("Server closed connection: ID=%d, ", map_int_int_it->first);

                                map_ID_sockets.erase(map_int_int_it);
                                map<int,CLIENT>::iterator map_int_client_it;
                                map_int_client_it=map_socket_clients.find(sock);
                                if(map_int_client_it!=map_socket_clients.end())
                                {
                                    if(map_int_client_it->second.state==0)
                                    {
                                        printf("USER ID%d This service's duration is end !!!\n\n",map_int_client_it->second.id);
                                    }
                                    else if(map_int_client_it->second.state==1)
                                    {
infoprint(map_socket_clients[sock].id,map_socket_clients[sock].hdf_type,map_socket_clients[sock].bss_type,map_socket_clients[sock].life_time);
                                        printf("USER ID%d bandwidth occupation,user  thrown out !!!\n\n",map_int_client_it->second.id);
                                    }
                                    else if(map_int_client_it->second.state==2)
                                    {
infoprint(map_socket_clients[sock].id,map_socket_clients[sock].hdf_type,map_socket_clients[sock].bss_type,map_socket_clients[sock].life_time);
                                        printf("USER ID%d remaining bandwidth is not enough, connection closed!!!\n\n",map_int_client_it->second.id);
                                    }
                                    else if(map_int_client_it->second.state==3)
                                    {
infoprint(map_socket_clients[sock].id,map_socket_clients[sock].hdf_type,map_socket_clients[sock].bss_type,map_socket_clients[sock].life_time);
                                        printf("USER ID%d Incorrect username or password ,connection failed!!!\n\n",map_int_client_it->second.id);
                                    }

                                    map_socket_clients.erase(map_int_client_it);
                                    bzero(buf_count,sizeof(buf_count));
                                    //sprintf(buf_count,"client count now = %lu\n",map_socket_clients.size());
                                    sprintf(buf_count,"Poisson arrival rate = %d\nclient count now = %lu\n",MINCLIENTSEC,map_socket_clients.size());
                                    res=write(pipecntonly,buf_count,sizeof(buf_count));
                                    if(res==-1)
                                    {
                                        fprintf(stderr, "Write error on pipe\n");
                                        //exit(EXIT_FAILURE);
                                    }
                                }


                                close(sock);//不知要不要这样重复关闭
                                delfd(epfd, sock, true);/////////////////////
                                //printf("Server closed connection: %d\n", sock);
                                //countnow--;
                                //printf("countnow=%d\n",countnow);

                                //printf("client count now = %lu\n\n\n",map_socket_clients.size());
                                bzero(buf_count,sizeof(buf_count));
                                //sprintf(buf_count,"client count now = %lu\n",map_socket_clients.size());
                                sprintf(buf_count,"Poisson arrival rate = %d\nclient count now = %lu\n",MINCLIENTSEC,map_socket_clients.size());
                                res=write(pipecntonly,buf_count,sizeof(buf_count));
                                if(res==-1)
                                {
                                    fprintf(stderr, "Write error on pipe\n");
                                    //exit(EXIT_FAILURE);
                                }
                            }

                        }
                        else
                        {
                            //printf("%s\n", message);//20160520
                            /* if(strcmp(message,"-2")==0)//timeout
                             {
                                 if(map_socket_clients.find(sock)!=map_socket_clients.end())
                                 {
                                     printf("The client id = %d is timeout !!!\n",map_socket_clients[sock].id);
                                 }
                             }
                             else */ if(strcmp(message,"-3")==0)//throw out for band
                            {
                                if(map_socket_clients.find(sock)!=map_socket_clients.end())
                                {
                                    //printf("The client id = %d has been thrown out !!!\n",map_socket_clients[sock].id);
                                    map_socket_clients[sock].state=1;
                                }
                            }
                            else if(strcmp(message,"-4")==0)//reject for band
                            {
                                if(map_socket_clients.find(sock)!=map_socket_clients.end())
                                {
                                    map_socket_clients[sock].state=2;
                                }
                            }
                            else if(strcmp(message,"-5")==0)//reject for db
                            {
                                if(map_socket_clients.find(sock)!=map_socket_clients.end())
                                {
                                    map_socket_clients[sock].state=3;
                                }
                            }
                            else  if(strcmp(message,"02")==0)//come in
                            {
                                if(map_socket_clients.find(sock)!=map_socket_clients.end())
                                {
                                    infoprint(map_socket_clients[sock].id,map_socket_clients[sock].hdf_type,map_socket_clients[sock].bss_type,map_socket_clients[sock].life_time);
                                    printf("USER ID: %d connect success!!!\n\n\n",map_socket_clients[sock].id);
                                    bzero(buf_count,sizeof(buf_count));
                                    sprintf(buf_count,"Poisson arrival rate = %d\nclient count now = %lu\n",MINCLIENTSEC,map_socket_clients.size());
                                    res=write(pipecntonly,buf_count,sizeof(buf_count));
                                    if(res==-1)
                                    {
                                        fprintf(stderr, "Write error on pipe\n");
                                        //exit(EXIT_FAILURE);
                                    }
                                }
                            }
                        }
                    }
                }//for
            }//while
            close(pipecntonly);

        }//else end

    }

    if(pid)
    {
        //关闭父进程和sock
        printf("close the father thread.\n");
        close(pipe_fd[0]);
        //close(sock);
    }
    else
    {
        //关闭子进程
        printf("close the son thread.\n");
        close(pipe_fd[1]);
    }

    return 0;
}





