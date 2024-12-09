import asyncio
import websockets

# set of connected clients
connected_clients = set()

async def echo(websocket):
    # add the new client
    remote_addr = websocket.remote_address
    connected_clients.add(websocket)
    print(f"new client {remote_addr[0]}:{remote_addr[1]} connected... [num clients: {len(connected_clients)}]")

    try:
        async for message in websocket:
            # broadcast the received message to all clients
            remote_addr = websocket.remote_address
            print(f"\tmessage from {remote_addr[0]}:{remote_addr[1]}: {message}")
            for client in connected_clients:
                #if client != websocket:  # when wants to exclude the sending host
                    await client.send(message)
    except websockets.exceptions.ConnectionClosed:
        print("exceptional connection closed....")

    finally:
        # remove the closed client
        remote_addr = websocket.remote_address
        connected_clients.remove(websocket)
        print(f"client({remote_addr[0]}:{remote_addr[1]}) closed... [num clients: {len(connected_clients)}]")

async def main():
    # run the WebSocket server
    # server_ip   = "192.168.0.12"
    server_ip   = "localhost"
    server_port = 9999
    async with websockets.serve(echo, server_ip, server_port):
        print(f"\nWebSocket server is excecuting at {server_ip}:{server_port}...\n")
        await asyncio.Future()  # keep the server waiting events

# event loop with asyncio.run()
if __name__ == "__main__":
    asyncio.run(main())
