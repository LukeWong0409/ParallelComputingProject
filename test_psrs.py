import subprocess
import time
import re
import os

DATA_SIZES = [1000, 5000, 10000, 100000, 1000000]
NUM_CLIENTS_LIST = [1, 2, 4]
PSRS_EXE = ".\\psrs_socket.exe"

serial_times = {}
parallel_times = {}
speedup_ratios = {}


def run_server(num_clients, data_size):
    """运行服务器进程"""
    cmd = f"{PSRS_EXE} server {num_clients} {data_size}"
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, text=True)


def run_clients(server_ip, num_clients):
    """运行所有客户端进程"""
    clients = []
    for client_id in range(num_clients):
        cmd = f"{PSRS_EXE} client {server_ip} {client_id} {num_clients}"
        client_process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, text=True)
        clients.append(client_process)
    return clients


def wait_for_processes(processes):
    """等待所有进程结束"""
    for process in processes:
        process.wait()


def extract_sort_time(server_process):
    """从服务器输出中提取排序时间"""
    output, error = server_process.communicate()
    if server_process.returncode != 0:
        print(f"服务器运行错误: {error}")
        return None
    
    match = re.search(r"总排序时间: ([\d.]+) 秒", output)
    if match:
        return float(match.group(1))
    else:
        print(f"无法提取排序时间，输出: {output}")
        return None


def run_test(num_clients, data_size):
    """执行单次测试"""
    print(f"测试: 数据量={data_size}, 客户端数={num_clients}")
    
    server = run_server(num_clients, data_size)
    time.sleep(1)
    
    clients = run_clients("127.0.0.1", num_clients)
    
    wait_for_processes(clients)
    
    sort_time = extract_sort_time(server)
    
    if sort_time is not None:
        print(f"  完成! 排序时间: {sort_time:.4f}秒")
    return sort_time


def main():
    print("=" * 70)
    print("PSRS并行排序算法 性能自动测试")
    print("=" * 70)
    
    print("\n1. 测试串行排序时间 (1个客户端):")
    print("-" * 50)
    for data_size in DATA_SIZES:
        serial_time = run_test(1, data_size)
        if serial_time is not None:
            serial_times[data_size] = serial_time
    
    print("\n2. 测试并行排序时间 (多个客户端):")
    print("-" * 50)
    for num_clients in NUM_CLIENTS_LIST[1:]:
        for data_size in DATA_SIZES:
            parallel_time = run_test(num_clients, data_size)
            if parallel_time is not None:
                if data_size not in parallel_times:
                    parallel_times[data_size] = {}
                parallel_times[data_size][num_clients] = parallel_time
    
    print("\n3. 计算加速比:")
    print("-" * 50)
    for data_size in DATA_SIZES:
        if data_size in serial_times:
            serial_time = serial_times[data_size]
            speedup_ratios[data_size] = {}
            
            for num_clients in NUM_CLIENTS_LIST[1:]:
                if data_size in parallel_times and num_clients in parallel_times[data_size]:
                    parallel_time = parallel_times[data_size][num_clients]
                    speedup = serial_time / parallel_time
                    speedup_ratios[data_size][num_clients] = speedup
                    print(f"  数据量={data_size}, {num_clients}客户端: 加速比={speedup:.2f}")
    
    print("\n" + "=" * 70)
    print("PSRS算法性能测试报告")
    print("=" * 70)
    print(f"{'数据量':<10} {'串行时间(秒)':<15} {'2客户端时间':<15} {'2客户端加速比':<15} {'4客户端时间':<15} {'4客户端加速比':<15}")
    print("-" * 70)
    
    for data_size in DATA_SIZES:
        serial_time = serial_times.get(data_size, "N/A")
        
        time_2 = parallel_times.get(data_size, {}).get(2, "N/A")
        speedup_2 = speedup_ratios.get(data_size, {}).get(2, "N/A")
        
        time_4 = parallel_times.get(data_size, {}).get(4, "N/A")
        speedup_4 = speedup_ratios.get(data_size, {}).get(4, "N/A")
        
        serial_str = f"{serial_time:.4f}" if serial_time != "N/A" else "N/A"
        time_2_str = f"{time_2:.4f}" if time_2 != "N/A" else "N/A"
        speedup_2_str = f"{speedup_2:.2f}" if speedup_2 != "N/A" else "N/A"
        time_4_str = f"{time_4:.4f}" if time_4 != "N/A" else "N/A"
        speedup_4_str = f"{speedup_4:.2f}" if speedup_4 != "N/A" else "N/A"
        
        print(f"{data_size:<10} {serial_str:<15} {time_2_str:<15} {speedup_2_str:<15} {time_4_str:<15} {speedup_4_str:<15}")
    
    print("=" * 70)
    print("测试完成!")


if __name__ == "__main__":
    main()
