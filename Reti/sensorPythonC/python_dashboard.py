import socket
import struct
import sys

def main():
    HOST = "127.0.0.1"
    PORT = 11000
    PACKET_SIZE = 12

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind((HOST, PORT))
        print(f"[*] Python Dashboard listening on UDP {HOST}:{PORT}...")
    except OSError as e:
        print(f"[-] Failed to bind to socket: {e}")
        sys.exit(1)

    try:
        while True:
            data, addr = sock.recvfrom(PACKET_SIZE)
            
            if len(data) != PACKET_SIZE:
                print(f"[!] Warning: Received {len(data)} bytes, expected {PACKET_SIZE} bytes.")
                continue
            
            # The ">" forces Network Byte Order (Big Endian). 
            # "i" = 4-byte integer, "f" = 4-byte float (IEEE 754).
            sensor_id, udp_port, value_sensor = struct.unpack(">i i f", data)
            
            print("-" * 30)
            print(f" [📡] INCOMING FROM C GATEWAY")
            print(f"  > Sensor ID : {sensor_id}")
            print(f"  > UDP Port  : {udp_port}")
            print(f"  > Value     : {value_sensor:.2f}")
            print("-" * 30)
            
    except KeyboardInterrupt:
        print("\n[*] Exiting Python Dashboard. Goodbye!")
    finally:
        sock.close()

if __name__ == "__main__":
    main()