import socket
import threading

def handle_client(client_socket, client_address):
    with client_socket:
        print(f"Connection from {client_address}")
        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            message = data.decode('utf-8')
            print(f"Received data: {message}")
            
            # Echo the received message back to the client
            client_socket.sendall(data)

            # Prompt for user input to send to the connected client
            user_input = input("Enter message to send: ")
            client_socket.sendall(user_input.encode('utf-8'))

def start_server(host='0.0.0.0', port=8080):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((host, port))
        server_socket.listen(5)
        print(f"Server listening on {host}:{port}")

        while True:
            client_socket, client_address = server_socket.accept()
            client_handler = threading.Thread(target=handle_client, args=(client_socket, client_address))
            client_handler.start()

if __name__ == "__main__":
    start_server()
