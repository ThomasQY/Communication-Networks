#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <queue>
#include <sstream>
#include <vector>
#include <climits>
#include <algorithm>
#include <math.h>


using namespace std;
#define N_NUM 100

class Node
{
public:
  int index;
  int backoff;
  int rand_range;
  int retry;
  void setRandom(int R);
};

void Node::setRandom(int R)
{
// can pick 0
  this->backoff = rand() % (R + 1);
}

int run_time = 50;
vector<int> N_arr;
vector<double> utilization_arr;
vector<double> idle_arr;
vector<double> collision_arr;


double every_collision_tot;
double every_utilize_tot;
double every_idle_tot;

int N,L,M,T; // N: node count ; L: pkt_size ; M: Maximum transmission attempt ; T: time of simulation
vector<int>R; // R: Rand range


// int findVar(vector<int> arr,int is_collision){
//     int var = 0;
//     int mean =0;
//     mean = is_collision ? total_collision_cnt/N : total_success_cnt/N;
//     for( int n = 0; n < N; n++ )    
//     {
//         var += ((arr[n] - mean) * (arr[n] - mean));
//     }
//     var /= N;
//     return var;
// }


// prepare node data structure 
void makeNodes(vector<Node>& nodes){
    int i;
    Node mynode;
    for(i = 0; i < N ; i++){
        mynode.index = i;
        mynode.backoff = 0;
        mynode.rand_range = R[mynode.backoff];
        mynode.retry = 0;
        nodes.push_back(mynode);
        mynode.setRandom(nodes[i].rand_range);
    }
    // cout << "here1" <<endl;
}

// set every node an initial random backoff 
void initNodes(vector<Node> & nodes){
    for(int i = 0 ;i < N; i++){
        nodes[i].setRandom(nodes[i].rand_range);
        // cout << "node "<< i << "init set to "<< nodes[i].backoff<<endl;
    }
}

// get all nodes which are ready to send pkt at this point
vector<int> getReadyNode(vector<Node> & nodes){
    vector<int> ret;
    for(int i =0 ;i < N; i++){
        if(nodes[i].backoff == 0 ) { 
            ret.push_back(nodes[i].index);
        }
    }
    return ret;
}

void collision(Node mynode){
    mynode.retry ++;
    if(mynode.retry == M - 1){
        // drop packet, reset R, collision, reset backoff
        mynode.retry = 0;
        mynode.rand_range = R[mynode.retry];
        mynode.setRandom(mynode.rand_range);
    }else{
        // double rand_range
        mynode.rand_range  = R[mynode.retry];
    }   
}

void simulate(int loop_cnt){
    int cnt = 0;
    // int every_utilize[100];
    // int every_collision[100];
    // int every_idle[100];
    every_utilize_tot = 0.0;
    every_collision_tot = 0.0;
    every_idle_tot = 0.0;
    while(cnt < run_time){
        long total_collision_cnt = 0;
        long total_success_cnt = 0;
        long channel_idle = 0;
        long channel_used = 0;
        long packet_send = 0;
        vector<Node>nodes;
        srand (time(NULL));
        int time = 0;
        makeNodes(nodes);
        int curr_pkt = 0; //remaining of current transmission
        int curr_transmit = -1; // node which is currently transmissting 
        while(time < T){
            if(curr_pkt > 0){
            // channel busy, freeze all count down and continue transmit
            channel_used ++;
            curr_pkt --;
            }else{
                //re-rand previously transmiited node 
                if(curr_transmit != -1){
                    total_success_cnt ++;
                    nodes[curr_transmit].setRandom(nodes[curr_transmit].rand_range);
                    // cout << "node " << curr_transmit << "done transmission; rerand to "<< nodes[curr_transmit].backoff<< endl;
                    curr_transmit = -1;
                }
                // channel idle, find new node to transmit
                vector<int> rdy = getReadyNode(nodes);
                if(rdy.size() == 0){
                    // no ready node
                    channel_idle ++;
                    for (int i = 0 ;i < N; i++){
                        if(nodes[i].backoff) nodes[i].backoff --;
                    }
                }else{
                    // this cycle counted as used 
                    channel_used ++;
                    // pick a random node from list
                    int lucky;
                    lucky = rand() % (rdy.size());
                    // cout << "pick node "<<rdy[lucky]<<endl;
                    curr_pkt = L;
                    curr_transmit = rdy[lucky];
                    for(int i : rdy){
                        if(nodes[i].index != lucky){
                            // fail to be picked, collision 
                            collision(nodes[i]);
                            total_collision_cnt ++;
                        }
                    }
                }
            }
            time ++;
        }   
            // find avg of this 100 run and output to vector
        //  every_collision[cnt] = total_collision_cnt;
         every_collision_tot += total_collision_cnt;
        //  every_utilize[cnt] = ((double)channel_used / T) * 100.0;
        // cout << "channel used is: " << channel_used <<endl;
        // cout << "channel used percent is : " << (double)channel_used/T <<endl;
        every_utilize_tot += ((double)channel_used / T);
        // cout << "channel utilize total is : " << every_utilize_tot <<endl;
        //  every_idle[cnt] = ((double)channel_idle / T) * 100.0;
         every_idle_tot += 1 - ((double)channel_used / T);
         cnt ++;
    }
    collision_arr.push_back(every_collision_tot/run_time);
    utilization_arr.push_back((double)every_utilize_tot/run_time);
    // cout << "utilization is : " << (double)every_utilize_tot/run_time << endl;
    idle_arr.push_back((double)every_idle_tot/run_time);
}

// partially from  https://github.com/f1mm/ece438/blob/master/mp4/src/csma.cpp
int main(int argc, char** argv){
    if (argc != 2)
	{
		fprintf(stderr, "usage: ./csma_report input.txt\n");
		exit(1);
	}

	char temp;

    // parse input

    ifstream input_(argv[1]);
    input_ >> temp >> N >> temp >> L >> temp;
    // cout << "N: "<< N << " L: "<< L << " R:";

    while (input_.get() != 10) { // while we are not at end of line
	    int backoff_time;
		input_ >> backoff_time;
        // cout << backoff_time << " ";
		R.push_back(backoff_time);
	}
    // cout << endl;
    input_ >> temp >> M >> temp >> T;
    // cout << "M: "<< M << " T: "<< T << endl;
    int i = 0;
    while(N <= 500){
        cout<< "N= "<< N<<endl;
         // loop for differnt N
            // for each set, loop 100 times
        simulate(N);
        N += 5;  
    }
    cout << "ready to output "<< endl;
    ofstream outfile;
    outfile.open("report.txt");
    outfile << "N: ";
    for(int i =0; i< N_NUM; i++){
        outfile << (i+1)*5 << " ";
    }
    outfile << endl;
    outfile << "Utilization: ";
    for(int i =0; i< N_NUM ; i++){
        outfile << (double)utilization_arr[i] << " ";
    }
    outfile << endl;
    outfile << "Idle: ";
    for(int i =0; i< N_NUM; i++){
        outfile << (double)idle_arr[i]<< " ";
    }
    outfile << endl;
    outfile << "Collision: ";
    for(int i =0; i< N_NUM; i++){
        outfile << collision_arr[i] << " ";
    }
    outfile << endl;
    outfile.close();
}






