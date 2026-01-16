# Dashsrv

Dashsrv is a dashboard server written in C++ to show a basic dashboard of all the servers running on my home server.
Mostly all of the hardcoded connections are removed and replaced with the `config.json` file (see how to configure below).

Note: our mongoose version is modified

If you came for the minecraft stuff, check out `include/Minecraft/` and `source/Minecraft/`.
All files in these should be mostly independent of the rest of the system.

## Configuration

Dashsrv is easy to configure, just add a file named `config.json` in the `resources/` directory. If it doesn't exist when Dashsrv is first ran,
it will be automatically generated.

Basic configuration:
```json
{
    "hostip": "0.0.0.0",
    "hostport": "80",
    "servers": [
        {
            "type": "minecraft",
            "ip": "mc.hypixel.net",
            "extra-domain": "mc.hypixel.net",
            "port": 25565,
            "version": 774
        },
        {
            "type": "dashboard",
            "ip": "127.0.0.1",
            "port": 80
        }
    ]
}
```

Configuration options:
- `hostip`: The ip to host on. Recommended and default is "0.0.0.0", but you can change this to "127.0.0.1" if you don't wish for the dashboard to be hosted on the LAN.
- `hostport`: The port to host on. For easy access, I recommend `80`. Defaults to `8080`. Do note that Linux will by default prevent serving on port `80`.
- `servers`: An array/list of all servers displayed by this dashboard, see below for a list of properties in each server object:
    - `type`: The type of server, valid values are `minecraft`, `jellyfin`, or `dashboard`. Must be lowercase.
        - `minecraft`: For including a Minecraft server in the dashboard (max of `1` server)
            - `ip`: The raw ip address of the Minecraft server. If your server is local, provide the ip address in whichever way works best (either the LAN ip such as `192.168.0.100` or as a LAN domain, such as `hostname.local` if using mDNS).
            - `extra-domain`: An extra domain name in case the IP doesn't match a clean URL. This is helpful for reverse proxied servers such as through playit, in which the local ip can be provided as `ip` and the playit domain can be provided here.
            - `port`: The port the minecraft server is hosted on. Vanilla default is `25565`.
            - `version`: The version of minecraft to request the server from. `774` is Minecraft version 1.21.11. It's not necessary that this match the server's version, do know though that servers can provide different version responses as they see fit, ex. if your version is unsupported, the server can respond "Unsupported" instead of "Version 1.21" or other.
        - `jellyfin`: For including a Jellyfin server in the dashboard (max of `1` server)
            - `ip`: The ip address of the Jellyfin server. Can either be a raw IP or domain name.
            - `port`: The port of the Jellyfin server. Jellyfin's default is `8096`.
        - `dashboard`: For including other servers running Dashsrv (no limit)
            - `ip`: The ip address or domain name of the Dashsrv server.
            - `port`: The port of the Dashsrv server. Dashsrv default port is `8080`.
