import socket
import struct
import threading
import sys
import time

GATEWAY_IP = "127.0.0.1"
GATEWAY_PORT = 6000

TYPE_REGISTER = 1
TYPE_REGISTER_ERROR = 2
TYPE_REGISTER_ACK = 3
TYPE_LOOKUP = 4

# > = Big Endian (Network Order)
# i = 4-byte int (type)
# 32s = 32-byte char array (sender)
# 32s = 32-byte char array (target)
# i = 4-byte int (port)
# 255s = 255-byte char array (payload)
PACKET_FORMAT = "> i 32s 32s i 255s"

def create_packet(p_type, sender, target, port, payload):
    return struct.pack(
        PACKET_FORMAT,
        p_type,
        sender.encode('utf-8').ljust(32, b'\x00'),
        target.encode('utf-8').ljust(32, b'\x00'),
        port,
        payload.encode('utf-8').ljust(255, b'\x00')
    )

def unpack_packet(data):
    unpacked = struct.unpack(PACKET_FORMAT, data)
    return {
        "type": unpacked[0],
        "sender": unpacked[1].decode('utf-8').rstrip('\x00'),
        "target": unpacked[2].decode('utf-8').rstrip('\x00'),
        "port": unpacked[3],
        "payload": unpacked[4].decode('utf-8').rstrip('\x00')
    }

def receive_thread(sock):
    while True:
        try:
            data, _ = sock.recvfrom(1024)
            packet = unpack_packet(data)
            
            if packet["type"] == TYPE_LOOKUP:
                print(f"\n\a[MESSAGGIO DA {packet['sender']}]: {packet['payload']}")
                print("> Destinatario: ", end="", flush=True)
            elif packet["type"] == TYPE_REGISTER_ERROR:
                print(f"\n[ERRORE DAL SERVER]: {packet['payload']}")
                print("> Destinatario: ", end="", flush=True)
        except Exception:
            break

def main():
    if len(sys.argv) != 3:
        print("Uso: python python_client.py <Nickname> <Porta>")
        sys.exit(1)

    nickname = sys.argv[1]
    port = int(sys.argv[2])

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind(("127.0.0.1", port))
    except OSError as e:
        print(f"[-] Errore bind porta: {e}")
        sys.exit(1)

    print("[SYSTEM] Registrazione al server in corso...")
    
    reg_packet = create_packet(TYPE_REGISTER, nickname, "Server", port, "REG_REQ")
    sock.sendto(reg_packet, (GATEWAY_IP, GATEWAY_PORT))

    data, _ = sock.recvfrom(1024)
    response = unpack_packet(data)

    if response["type"] == TYPE_REGISTER_ERROR:
        print(f"[FATALE] Registrazione fallita: {response['payload']}")
        sock.close()
        sys.exit(1)

    print(f"[SYSTEM] Autenticato come '{nickname}' sulla porta {port}!\n")

    # Avvio il thread per ricevere
    t = threading.Thread(target=receive_thread, args=(sock,), daemon=True)
    t.start()

    time.sleep(0.1)

    while True:
        try:
            target = input("\n> Destinatario: ").strip()
            if not target:
                continue
            message = input("> Messaggio: ").strip()

            msg_packet = create_packet(TYPE_LOOKUP, nickname, target, port, message)
            sock.sendto(msg_packet, (GATEWAY_IP, GATEWAY_PORT))
        except KeyboardInterrupt:
            print("\nChiusura client...")
            sock.close()
            sys.exit(0)

if __name__ == "__main__":
    main()