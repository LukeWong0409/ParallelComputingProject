#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <string>
#include <cstring>
#include <queue>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace std::chrono;

const int PORT = 12345;
const int BUFFER_SIZE = 1024;

vector<int> generate_random_array(int size) {
    vector<int> arr(size);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 1000000);
    
    for (int i = 0; i < size; i++) {
        arr[i] = dis(gen);
    }
    
    return arr;
}

class SocketComm {
private:
    SOCKET sock;
    sockaddr_in addr;
    bool is_server;

public:
    vector<SOCKET> client_socks;
    
    SocketComm() : sock(INVALID_SOCKET), is_server(false) {}
    
    ~SocketComm() {
        cleanup();
    }
    
    bool init() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            cout << "WSAStartup failed: " << result << endl;
            return false;
        }
        return true;
    }
    
    bool create_server(int num_clients) {
        is_server = true;
        
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            cout << "socket failed: " << WSAGetLastError() << endl;
            return false;
        }
        
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(PORT);
        
        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            cout << "bind failed: " << WSAGetLastError() << endl;
            closesocket(sock);
            return false;
        }
        
        if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
            cout << "listen failed: " << WSAGetLastError() << endl;
            closesocket(sock);
            return false;
        }
        
        cout << "服务器启动，等待客户端连接..." << endl;
        
        for (int i = 0; i < num_clients; i++) {
            SOCKET client_sock = accept(sock, NULL, NULL);
            if (client_sock == INVALID_SOCKET) {
                cout << "accept failed: " << WSAGetLastError() << endl;
                continue;
            }
            
            client_socks.push_back(client_sock);
            cout << "客户端 " << i << " 连接成功" << endl;
        }
        
        return true;
    }
    
    bool connect_to_server(const char* server_ip) {
        is_server = false;
        
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            cout << "socket failed: " << WSAGetLastError() << endl;
            return false;
        }
        
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = inet_addr(server_ip);
        
        if (addr.sin_addr.s_addr == INADDR_NONE) {
            cout << "无效的服务器地址" << endl;
            closesocket(sock);
            return false;
        }
        
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            cout << "connect failed: " << WSAGetLastError() << endl;
            closesocket(sock);
            return false;
        }
        
        cout << "连接到服务器成功" << endl;
        return true;
    }
    
    bool send_vector(const vector<int>& vec, SOCKET target_sock = INVALID_SOCKET) {
        SOCKET send_sock = (target_sock == INVALID_SOCKET) ? sock : target_sock;
        
        int size = vec.size();
        if (send(send_sock, (char*)&size, sizeof(int), 0) == SOCKET_ERROR) {
            cout << "send size failed: " << WSAGetLastError() << endl;
            return false;
        }
        
        if (size > 0) {
            if (send(send_sock, (char*)vec.data(), size * sizeof(int), 0) == SOCKET_ERROR) {
                cout << "send data failed: " << WSAGetLastError() << endl;
                return false;
            }
        }
        
        return true;
    }
    
    bool receive_vector(vector<int>& vec, SOCKET source_sock = INVALID_SOCKET) {
        SOCKET recv_sock = (source_sock == INVALID_SOCKET) ? sock : source_sock;
        
        int size;
        if (recv(recv_sock, (char*)&size, sizeof(int), 0) == SOCKET_ERROR) {
            cout << "recv size failed: " << WSAGetLastError() << endl;
            return false;
        }
        
        vec.resize(size);
        if (size > 0) {
            int total_received = 0;
            while (total_received < size * sizeof(int)) {
                int received = recv(recv_sock, (char*)vec.data() + total_received, 
                                  size * sizeof(int) - total_received, 0);
                if (received == SOCKET_ERROR) {
                    cout << "recv data failed: " << WSAGetLastError() << endl;
                    return false;
                }
                total_received += received;
            }
        }
        
        return true;
    }
    
    void broadcast_vector(const vector<int>& vec) {
        for (SOCKET client_sock : client_socks) {
            send_vector(vec, client_sock);
        }
    }
    
    vector<vector<int>> receive_all_vectors() {
        vector<vector<int>> vectors;
        for (SOCKET client_sock : client_socks) {
            vector<int> vec;
            receive_vector(vec, client_sock);
            vectors.push_back(vec);
        }
        return vectors;
    }
    
    void cleanup() {
        if (is_server) {
            for (SOCKET client_sock : client_socks) {
                closesocket(client_sock);
            }
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
            }
        } else {
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
            }
        }
        WSACleanup();
    }
};

void psrs_server(int num_clients, int data_size) {
    SocketComm comm;
    if (!comm.init()) {
        return;
    }
    
    if (!comm.create_server(num_clients)) {
        comm.cleanup();
        return;
    }
    
    vector<int> arr = generate_random_array(data_size);
    cout << "生成数据完成，大小: " << data_size << endl;
    
    int local_n = data_size / num_clients;
    int remainder = data_size % num_clients;
    
    vector<vector<int>> partitions(num_clients);
    int index = 0;
    for (int i = 0; i < num_clients; i++) {
        int part_size = local_n + (i < remainder ? 1 : 0);
        partitions[i] = vector<int>(arr.begin() + index, arr.begin() + index + part_size);
        index += part_size;
    }
    
    cout << "发送数据到客户端..." << endl;
    for (int i = 0; i < num_clients; i++) {
        comm.send_vector(partitions[i], comm.client_socks[i]);
    }
    
    cout << "接收客户端样本..." << endl;
    vector<vector<int>> all_samples = comm.receive_all_vectors();
    
    vector<int> merged_samples;
    for (const auto& samples : all_samples) {
        merged_samples.insert(merged_samples.end(), samples.begin(), samples.end());
    }
    
    sort(merged_samples.begin(), merged_samples.end());
    
    vector<int> pivots(num_clients - 1);
    for (int i = 0; i < num_clients - 1; i++) {
        pivots[i] = merged_samples[(i + 1) * (merged_samples.size() / (num_clients))];
    }
    
    cout << "广播主元到客户端..." << endl;
    comm.broadcast_vector(pivots);
    
    cout << "接收客户端重新划分的数据段..." << endl;
    vector<vector<vector<int>>> all_segments(num_clients);
    for (int i = 0; i < num_clients; i++) {
        all_segments[i] = comm.receive_all_vectors();
    }
    
    cout << "归并排序..." << endl;
    vector<int> final_result;
    
    vector<vector<int>> segments_to_merge;
    for (int i = 0; i < num_clients; i++) {
        vector<int> combined;
        for (int j = 0; j < num_clients; j++) {
            combined.insert(combined.end(), all_segments[j][i].begin(), all_segments[j][i].end());
        }
        sort(combined.begin(), combined.end());
        if (!combined.empty()) {
            segments_to_merge.push_back(combined);
        }
    }
    
    typedef pair<int, pair<int, int>> HeapElement;
    std::priority_queue<HeapElement, std::vector<HeapElement>, std::greater<HeapElement>> min_heap;
    
    for (int i = 0; i < segments_to_merge.size(); i++) {
        if (!segments_to_merge[i].empty()) {
            min_heap.push({segments_to_merge[i][0], {i, 0}});
        }
    }
    
    while (!min_heap.empty()) {
        HeapElement current = min_heap.top();
        min_heap.pop();
        
        int value = current.first;
        int segment_idx = current.second.first;
        int element_idx = current.second.second;
        
        final_result.push_back(value);
        
        if (element_idx + 1 < segments_to_merge[segment_idx].size()) {
            min_heap.push({segments_to_merge[segment_idx][element_idx + 1], 
                          {segment_idx, element_idx + 1}});
        }
    }
    
    bool is_sorted = true;
    for (int i = 1; i < final_result.size(); i++) {
        if (final_result[i] < final_result[i-1]) {
            is_sorted = false;
            break;
        }
    }
    
    if (is_sorted) {
        cout << "排序结果正确!" << endl;
    } else {
        cout << "排序结果错误!" << endl;
    }
    
    comm.cleanup();
}

void psrs_client(const char* server_ip, int client_id, int num_clients) {
    SocketComm comm;
    if (!comm.init()) {
        return;
    }
    
    if (!comm.connect_to_server(server_ip)) {
        comm.cleanup();
        return;
    }
    
    vector<int> local_data;
    comm.receive_vector(local_data);
    
    sort(local_data.begin(), local_data.end());
    
    vector<int> samples(num_clients - 1);
    int sample_step = local_data.size() / num_clients;
    for (int i = 0; i < num_clients - 1; i++) {
        samples[i] = local_data[(i + 1) * sample_step];
    }
    
    comm.send_vector(samples);
    
    vector<int> pivots;
    comm.receive_vector(pivots);
    
    vector<vector<int>> segments(num_clients);
    int pivot_index = 0;
    for (int num : local_data) {
        while (pivot_index < pivots.size() && num > pivots[pivot_index]) {
            pivot_index++;
        }
        segments[pivot_index].push_back(num);
    }
    
    for (const auto& segment : segments) {
        comm.send_vector(segment);
    }
    
    comm.cleanup();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "用法: " << endl;
        cout << "  服务器: " << argv[0] << " server <客户端数量> <数据大小>" << endl;
        cout << "  客户端: " << argv[0] << " client <服务器IP> <客户端ID> <客户端总数>" << endl;
        return 1;
    }
    
    string mode = argv[1];
    
    if (mode == "server") {
        if (argc != 4) {
            cout << "服务器用法: " << argv[0] << " server <客户端数量> <数据大小>" << endl;
            return 1;
        }
        
        int num_clients = stoi(argv[2]);
        int data_size = stoi(argv[3]);
        
        auto start = high_resolution_clock::now();
        psrs_server(num_clients, data_size);
        auto end = high_resolution_clock::now();
        
        double elapsed_time = duration<double>(end - start).count();
        cout << "总排序时间: " << elapsed_time << " 秒" << endl;
        
    } else if (mode == "client") {
        if (argc != 5) {
            cout << "客户端用法: " << argv[0] << " client <服务器IP> <客户端ID> <客户端总数>" << endl;
            return 1;
        }
        
        const char* server_ip = argv[2];
        int client_id = stoi(argv[3]);
        int num_clients = stoi(argv[4]);
        
        psrs_client(server_ip, client_id, num_clients);
        
    } else {
        cout << "无效的模式: " << mode << endl;
        cout << "支持的模式: server, client" << endl;
        return 1;
    }
    
    return 0;
}