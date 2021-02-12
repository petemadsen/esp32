try:
    import uasyncio as asyncio
except:
    import asyncio

import time


async def neopixel_task():
    while True:
        print("neopixel")
        await asyncio.sleep(1)


async def handle_request(reader, writer):
    bytes_in = await reader.readline()
    print("request: %s" % bytes_in)
    print(writer.get_extra_info("peername"))
#    reader.close()

    writer.write(b"thank you\r\n")
    await writer.drain()
    writer.close()
#    await writer.wait_closed()


async def http_server_task():
    await asyncio.start_server(handle_request, "0.0.0.0", "8080")


async def main():
#    server = await asyncio.start_server(handle_request, "0.0.0.0", "8080")
#    addr = server.sockets[0].getsockname()
#    print("Server on ", addr)

#    async with server:
#        await server.serve_forever()

    asyncio.create_task(neopixel_task())
    asyncio.create_task(http_server_task())
    await asyncio.sleep(30)


asyncio.run(main())

