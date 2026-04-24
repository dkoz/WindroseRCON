import socket
import struct

RCON_HOST = "127.0.0.1"
RCON_PORT = 27065
RCON_PASSWORD = "windrose_admin"

class RCONClient:
    SERVERDATA_AUTH = 3
    SERVERDATA_AUTH_RESPONSE = 2
    SERVERDATA_EXECCOMMAND = 2
    SERVERDATA_RESPONSE_VALUE = 0

    def __init__(self, host, port, password):
        self.host = host
        self.port = port
        self.password = password
        self.sock = None
        self.request_id = 0

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        return self.authenticate()

    def disconnect(self):
        if self.sock:
            self.sock.close()
            self.sock = None

    def send_packet(self, packet_type, body):
        self.request_id += 1
        body_bytes = body.encode('utf-8') + b'\x00'
        packet_size = len(body_bytes) + 10
        
        packet = struct.pack('<i', packet_size)
        packet += struct.pack('<i', self.request_id)
        packet += struct.pack('<i', packet_type)
        packet += body_bytes
        packet += b'\x00'
        
        self.sock.sendall(packet)
        return self.request_id

    def receive_packet(self):
        size_data = self.sock.recv(4)
        if len(size_data) < 4:
            return None
        
        size = struct.unpack('<i', size_data)[0]
        data = b''
        while len(data) < size:
            chunk = self.sock.recv(size - len(data))
            if not chunk:
                return None
            data += chunk
        
        req_id = struct.unpack('<i', data[0:4])[0]
        pkt_type = struct.unpack('<i', data[4:8])[0]
        body = data[8:-2].decode('utf-8', errors='ignore')
        
        return {
            'id': req_id,
            'type': pkt_type,
            'body': body
        }

    def authenticate(self):
        self.send_packet(self.SERVERDATA_AUTH, self.password)
        response = self.receive_packet()
        
        if response and response['id'] != -1:
            print("✓ Authentication successful")
            return True
        else:
            print("✗ Authentication failed")
            return False

    def execute_command(self, command):
        self.send_packet(self.SERVERDATA_EXECCOMMAND, command)
        response = self.receive_packet()
        
        if response:
            return response['body']
        return None

def main():
    print("=" * 50)
    print("Windrose RCON Test Client")
    print("=" * 50)
    print(f"Connecting to {RCON_HOST}:{RCON_PORT}...")
    
    try:
        client = RCONClient(RCON_HOST, RCON_PORT, RCON_PASSWORD)
        client.connect()
        
        print("\nRCON Console Ready. Type 'quit' to exit.\n")
        
        while True:
            try:
                command = input("RCON> ")
                if command.lower() in ['quit', 'exit']:
                    break
                
                if command.strip():
                    response = client.execute_command(command)
                    if response:
                        print(f"Response: {response}")
                    else:
                        print("No response received")
            except KeyboardInterrupt:
                break
        
        client.disconnect()
        print("\nDisconnected.")
        
    except ConnectionRefusedError:
        print("Connection refused. Make sure the server is running.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
