import socket
import struct
import sys
import os

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def main():
    HOST = "127.0.0.1"
    PORT = 12000 # La porta in cui il Server C spara la telemetria
    
    PACKET_FORMAT = "> i i i i i"
    PACKET_SIZE = struct.calcsize(PACKET_FORMAT)
    
    players_status = {}

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind((HOST, PORT))
        print(f"[*] Battle Royale Observer Dashboard in ascolto su UDP {HOST}:{PORT}")
    except OSError as e:
        print(f"[-] Errore nell'avvio della dashboard: {e}")
        sys.exit(1)

    try:
        while True:
            data, _ = sock.recvfrom(1024)
            
            if len(data) >= PACKET_SIZE:
                p_type, p_id, p_x, p_y, hp = struct.unpack(PACKET_FORMAT, data[:PACKET_SIZE])
                
                # Se è un pacchetto vita (TYPE_UDP_LIFE = 2) aggiorniamo la dashboard
                if p_type == 2:
                    players_status[p_id] = {"x": p_x, "y": p_y, "hp": hp}
                    
                    clear_screen()
                    print("=" * 40)
                    print(" 🏆 BATTLE ROYALE - LIVE DASHBOARD 🏆")
                    print("=" * 40)
                    
                    for pid, stats in sorted(players_status.items()):
                        status = "ALIVE" if stats['hp'] > 0 else "DEAD 💀"
                        hp_bar = "█" * (stats['hp'] // 10)
                        
                        print(f" Player [{pid:02d}] - {status:7} | Pos: ({stats['x']:02d},{stats['y']:02d})")
                        print(f" HP [{stats['hp']:03d}]: |{hp_bar.ljust(10, ' ')}|")
                        print("-" * 40)

    except KeyboardInterrupt:
        print("\n[*] Uscita dalla dashboard...")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
