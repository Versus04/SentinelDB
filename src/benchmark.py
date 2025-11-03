# benchmark.py
import socket
import time
import threading

def benchmark_set(num_ops):
    """Benchmark SET operations"""
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(("localhost", 8080))
    
    start = time.time()
    for i in range(num_ops):
        cmd = f"SET key{i} value{i}\r\n"
        client.sendall(cmd.encode())
        response = client.recv(1024)
    end = time.time()
    
    client.sendall(b"EXIT\r\n")
    client.close()
    
    duration = end - start
    ops_per_sec = num_ops / duration
    print(f"SET: {num_ops} ops in {duration:.2f}s = {ops_per_sec:.2f} ops/sec")

def benchmark_get(num_ops):
    """Benchmark GET operations"""
    # First populate data
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(("localhost", 8080))
    
    for i in range(num_ops):
        cmd = f"SET key{i} value{i}\r\n"
        client.sendall(cmd.encode())
        client.recv(1024)
    
    # Now benchmark GET
    start = time.time()
    for i in range(num_ops):
        cmd = f"GET key{i}\r\n"
        client.sendall(cmd.encode())
        response = client.recv(1024)
    end = time.time()
    
    client.sendall(b"EXIT\r\n")
    client.close()
    
    duration = end - start
    ops_per_sec = num_ops / duration
    print(f"GET: {num_ops} ops in {duration:.2f}s = {ops_per_sec:.2f} ops/sec")

def concurrent_benchmark(num_clients, ops_per_client):
    """Benchmark with multiple concurrent clients"""
    def client_thread():
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect(("localhost", 8080))
        
        for i in range(ops_per_client):
            cmd = f"SET key{i} value{i}\r\n"
            client.sendall(cmd.encode())
            client.recv(1024)
        
        client.sendall(b"EXIT\r\n")
        client.close()
    
    threads = []
    start = time.time()
    
    for _ in range(num_clients):
        t = threading.Thread(target=client_thread)
        t.start()
        threads.append(t)
    
    for t in threads:
        t.join()
    
    end = time.time()
    duration = end - start
    total_ops = num_clients * ops_per_client
    ops_per_sec = total_ops / duration
    
    print(f"\nConcurrent ({num_clients} clients, {ops_per_client} ops each):")
    print(f"  Total: {total_ops} ops in {duration:.2f}s = {ops_per_sec:.2f} ops/sec")

if __name__ == "__main__":
    print("=== SentinelDB Benchmark ===\n")
    
    # Single client benchmarks
    benchmark_set(10000)
    benchmark_get(10000)
    
    # Concurrent benchmarks
    concurrent_benchmark(10, 1000)
    concurrent_benchmark(50, 1000)
    concurrent_benchmark(100, 1000)
