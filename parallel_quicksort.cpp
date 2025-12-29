#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <omp.h>

using namespace std;
using namespace std::chrono;

void quicksort_serial(vector<int>& arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high];
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                swap(arr[i], arr[j]);
            }
        }
        swap(arr[i + 1], arr[high]);
        int pi = i + 1;
        
        quicksort_serial(arr, low, pi - 1);
        quicksort_serial(arr, pi + 1, high);
    }
}

void quicksort_parallel(vector<int>& arr, int low, int high, int depth = 0) {
    if (low < high) {
        int pivot = arr[high];
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                swap(arr[i], arr[j]);
            }
        }
        swap(arr[i + 1], arr[high]);
        int pi = i + 1;
        
        if (depth < 4) {
            #pragma omp parallel sections
            {
                #pragma omp section
                quicksort_parallel(arr, low, pi - 1, depth + 1);
                
                #pragma omp section
                quicksort_parallel(arr, pi + 1, high, depth + 1);
            }
        } else {
            quicksort_serial(arr, low, pi - 1);
            quicksort_serial(arr, pi + 1, high);
        }
    }
}

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

void test_performance(int data_size, int num_threads) {
    cout << "测试数据量: " << data_size << "，线程数: " << num_threads << endl;
    
    omp_set_num_threads(num_threads);
    
    vector<int> arr_serial = generate_random_array(data_size);
    vector<int> arr_parallel = arr_serial;
    
    auto start_serial = steady_clock::now();
    quicksort_serial(arr_serial, 0, arr_serial.size() - 1);
    auto end_serial = steady_clock::now();
    double time_serial = duration<double>(end_serial - start_serial).count();
    
    auto start_parallel = steady_clock::now();
    quicksort_parallel(arr_parallel, 0, arr_parallel.size() - 1);
    auto end_parallel = steady_clock::now();
    double time_parallel = duration<double>(end_parallel - start_parallel).count();
    
    bool correct = is_sorted(arr_serial.begin(), arr_serial.end()) && 
                   is_sorted(arr_parallel.begin(), arr_parallel.end()) &&
                   arr_serial == arr_parallel;
    
    if (correct) {
        cout << "排序结果正确!" << endl;
        cout << "串行排序时间: " << time_serial << " 秒" << endl;
        cout << "并行排序时间: " << time_parallel << " 秒" << endl;
        cout << "加速比: " << time_serial / time_parallel << endl;
    } else {
        cout << "排序结果错误!" << endl;
    }
    
    cout << "------------------------" << endl;
}

int main() {
    vector<int> data_sizes = {1000, 5000, 10000, 100000};
    
    vector<int> thread_counts = {1, 2, 4, 8, 16};
    
    cout << "并行快速排序算法性能测试" << endl;
    cout << "==========================" << endl;
    
    for (int data_size : data_sizes) {
        cout << "\n数据量: " << data_size << endl;
        cout << "------------------------" << endl;
        
        for (int threads : thread_counts) {
            test_performance(data_size, threads);
        }
    }
    
    return 0;
}