#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <string>
#include <utility>
#include <set>
#include <limits>
#include <queue>
#include <stack> 
using namespace std;

int getSize(string topoFile);
vector<pair<int, int> > getDistanceVector(vector<vector<int> > adjMatrix, int maxNode, int startIndex);
void printForwardTable(vector<vector<int> > adjMatrix, int maxNode);
void printMessages(vector<vector<int> > adjMatrix, string messageFile, int maxNode);
ofstream output;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Usage: ./linkstate topofile messagefile changesfile" << endl;
        return -1;
    }
    output.open("output.txt");
    vector<string> allArgs(argv, argv + argc);
    string topoFile = allArgs[1];
    string messageFile = allArgs[2];
    string changesFile = allArgs[3];
    int maxNode = getSize(topoFile);
    // initializing adjacency matrix
    vector<vector<int> > adjMatrix(maxNode, vector<int> (maxNode, 0));

    for (int i = 0; i < maxNode; i++) {
        // adjMatrix[i] = vector<int>(maxNode);
        for (int j = 0; j < maxNode; j++) {
            adjMatrix[i][j] = -1;
            if (i == j) {
                // cost is 0
                adjMatrix[i][j] = 0;
            }
        }
    }
    // building adjacency matrix
    string line;
    ifstream File;
    File.open(topoFile);
    while (getline(File, line))
    {
        istringstream iss(line);
        int start, end, cost;
        if (!(iss >> start >> end >> cost)) { break; } // error
        // matrix is 0-indexed
        start--;
        end--;
        // undirected graph
        adjMatrix[start][end] = cost;
        adjMatrix[end][start] = cost;
    }

    // printing forward table (first time)
    printForwardTable(adjMatrix, maxNode);

    // dealing with messages (first time)
    printMessages(adjMatrix, messageFile, maxNode);

    // apply changes
    File.close();
    File.open(changesFile);
    while (getline(File, line))
    {
        istringstream iss(line);
        int start, end, cost;
        if (!(iss >> start >> end >> cost)) { break; } // error
        // modifying edges in the adjacenty matrix
        start--;
        end--;
        if (cost == -999) {
            // remove edge
            adjMatrix[start][end] = -1;
            adjMatrix[end][start] = -1;
        } else {
            adjMatrix[start][end] = cost;
            adjMatrix[end][start] = cost;
        } 
        // reprinting fowardtable and messages
        printForwardTable(adjMatrix, maxNode);
        printMessages(adjMatrix, messageFile, maxNode);

    }
    return 0;
}

int getSize(string topoFile) {
    string line;
    ifstream File;
    File.open(topoFile);
    int maxNode = -1;
    while (getline(File, line))
    {
        istringstream iss(line);
        int start, end, cost;
        if (!(iss >> start >> end >> cost)) { break; } // error
        maxNode = max(maxNode, start);
        maxNode = max(maxNode, end);
    }
    return maxNode;
}

vector<pair<int, int> > getDistanceVector(vector<vector<int> > adjMatrix, int maxNode, int startIndex) {
    set<int> visited;
    vector<pair<int, int> > dstVector(maxNode);
    // first means the last previous node, second means the total cost
    for (int i = 0; i < maxNode; i++) {
        // inifinity
        dstVector[i].first = -1;
        dstVector[i].second = numeric_limits<int>::max();
        // cost is 0
        if (i == startIndex) {
            dstVector[i].first = i;
            dstVector[i].second = 0;
        } else if (adjMatrix[startIndex][i] > -1) {
            dstVector[i].first = startIndex;
            dstVector[i].second = adjMatrix[startIndex][i];
        }
    }
    
    visited.insert(startIndex);
    while (visited.size() < maxNode) {
        // find min dst index
        int minDstIndex = -1;
        int minDst = numeric_limits<int>::max();
        for (int i = 0; i < maxNode; i++) {
            if (visited.find(i) == visited.end() && dstVector[i].second <= minDst) {
                minDst = dstVector[i].second;
                minDstIndex = i;
            }
        }
        if (minDst == numeric_limits<int>::max()) {
            // reamaining nodes are all unreachable
            break;
        }
        int currentIndex = minDstIndex;
        visited.insert(currentIndex);
        for (int i = 0; i < maxNode; i++) {
            if (visited.find(i) == visited.end() && adjMatrix[currentIndex][i] > -1) {
                // is neighbor and not visited, update dst if possible
                if (dstVector[i].second > dstVector[currentIndex].second + adjMatrix[currentIndex][i] || (dstVector[i].second == (dstVector[currentIndex].second + adjMatrix[currentIndex][i]) && currentIndex < dstVector[i].first)) {
                    // handles tie breaking as well
                    dstVector[i].first = currentIndex;
                    dstVector[i].second = dstVector[currentIndex].second + adjMatrix[currentIndex][i];
                } 
            }
        }
    }
    return dstVector;
    
}

void printForwardTable(vector<vector<int> > adjMatrix, int maxNode) {
    for (int i = 0; i < maxNode; i++) {
        vector<pair<int, int> > dstVector = getDistanceVector(adjMatrix, maxNode, i);
        for (int end = 0; end < dstVector.size(); end++) {
            if (dstVector[end].second == numeric_limits<int>::max()) {
                // skip unreachable entries
                continue;
            }
            int prev = end;
            int current = dstVector[end].first;
            int nextHop = -1;
            while (current != i) {
                prev = current;
                current = dstVector[current].first;
            }
            nextHop = prev;
            output << end + 1 << " " << nextHop + 1 << " " << dstVector[end].second << endl;
        }
    }
}

void printMessages(vector<vector<int> > adjMatrix, string messageFile, int maxNode) {
    // read messages
    string line;
    ifstream File;
    File.open(messageFile);
    while (getline(File, line))
    {
        string temp;
        istringstream iss(line);
        int start, end;
        string message;
        iss >> start;
        getline(iss, temp, ' ');
        iss >> end;
        getline(iss, temp, ' ');
        getline(iss, message, '\n');

        start--;
        end--;
        // dealing with one message
        vector<pair<int, int> > dstVector = getDistanceVector(adjMatrix, maxNode, start);
        if (dstVector[end].second == numeric_limits<int>::max()) {
            // unreachable
            output << "from " << start + 1 << " to " << end + 1 << " cost infinite hops unreachable " << "message " << message << endl;
            continue;
        }
        stack<int> path;
        int prev = end;
        path.push(prev);
        int current = dstVector[end].first;
        path.push(current);
        while (current != start) {
            prev = current;
            current = dstVector[current].first;
            path.push(current);
        }
        output << "from " << start + 1 << " to " << end + 1 << " cost " << dstVector[end].second << " hops ";
        while(!path.empty()) {
            int current = path.top();
            path.pop();
            if (!path.empty()) {
                output << current + 1 << " ";
            }
        }
        output << "message " << message << endl;

    }
    
}
