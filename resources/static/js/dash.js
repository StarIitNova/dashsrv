const IP = document.getElementById("current-ip");

const MCCacheTimer = document.getElementById("mc-cache-timer");
const MCIcon = document.getElementById("mc-server-icon");
const MCTitle = document.getElementById("mc-server-title");
const MCMOTD = document.getElementById("mc-server-motd");
const MCPlayers = document.getElementById("mc-players");
const MCPing = document.getElementById("mc-ping");

const JFCacheTimer = document.getElementById("jellyfin-cache-timer");
const JFName = document.getElementById("jellyfin-server-name");
const JFProductName = document.getElementById("jellyfin-product-name");
const JFLocation = document.getElementById("jellyfin-location");
const JFID = document.getElementById("jellyfin-id");
const JFVersion = document.getElementById("jellyfin-version");

const Servers = document.getElementById("servers");

async function APIGet(url) {
    try {
        const req = await fetch(url);
        if (!req.ok) {
            throw new Error('Network response was not ok ' + response.status);
        }

        return { data: await req.json(), error: null, success: true };
    } catch(err) {
        return { data: {}, error: err, success: false };
    }
}

function Datify(ms) {
    const dateCache = new Date(ms);
    const dateOptions = {
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
        hour12: false,
        timeZoneName: 'short'
    };

    return dateCache.toLocaleString(undefined, dateOptions);
}

async function updateServerList() {
    const data = await APIGet("/api/status");
    if (!data.success) {
        setTimeout(updateServerList, 30500);
        return;
    }

    Servers.innerHTML = "";

    let hasFailure = false;
    for (const server of data.data.data) {
        const div = document.createElement("div");
        div.classList.add("server");

        let ip = "";
        for (const opt of server.ips) {
            if (opt != "127.0.0.1" && opt != "localhost") {
                ip = opt;
                if (opt.startsWith("192.")) break;
            }
        }
        
        if (server.online) {
            div.innerHTML = `<h2 class="server-ip">${ip} ${server.self ? "<i class=\"server-current\">(current)</i>" : ""}</h2>
                             <p class="server-ping">${server.ping}ms</p><p class="server-cpu">CPU: ${server.cpu.toFixed(2)}%</p><p class="server-mem">MEM: ${(server.memory.usage * 100).toFixed(2)}%</p>`
        } else {
            hasFailure = true;
            div.innerHTML = `<h2 class="server-ip">${ip} ${server.self ? "<i class=\"server-current\">(current)</i>" : ""}</h2>
                             <p class="server-ping">Offline</p`
        }

        Servers.appendChild(div);
    }

    if (hasFailure) {
        setTimeout(updateServerList, 15500);
        return;
    }

    setTimeout(updateServerList, 5500);
    return;
}

async function updateMCServerInfo() {
    const data = await APIGet("/api/mc");
    if (!data.success) {
        return;
    }

    MCCacheTimer.innerText = Datify(data.data.cacheTiming);

    if (data.data.online) {
        MCIcon.src = data.data.icon;
        MCTitle.innerText = `KittenCraft (${data.data.version.name})`;
        MCMOTD.innerText = data.data.motd;
        MCMOTD.classList.remove("mc-motd-disconnected");
        MCPlayers.innerText = `${data.data.players.online}/${data.data.players.max}`
        MCPing.innerText = `${data.data.ping}ms`
    } else {
        MCMOTD.classList.add("mc-motd-disconnected");
        MCMOTD.innerText = `Can't connect to server.`;
        MCTitle.innerText = `Minecraft Server`;
        MCPlayers.innerText = `???`;
        MCPing.innerText = `X`;
        MCIcon.src = `#`;
    }
}

async function updateJellyfinInfo() {
    const data = await APIGet("/api/jellyfin");
    if (!data.success) {
        return;
    }

    JFCacheTimer.innerText = Datify(data.data.cacheTiming);
    
    if (data.data.online) {
        JFName.innerHTML = `${data.data.serverName} <i id="jellyfin-product-name">${data.data.productName}</i>`;
        JFLocation.innerText = data.data.localAddress;
        JFLocation.href = data.data.localAddress;
        JFID.innerText = data.data.id;
        JFVersion.innerText = data.data.version;

        if (data.data.healthString == "Healthy") {
            JFName.style = "color: #00ff00;";
        } else {
            JFName.style = "color: #ffff00;";
        }
    } else {
        JFName.innerText = "Offline";
        JFName.style = "color: #ff0000;";
        JFLocation.innerText = "";
        JFLocation.href = "#";
        JFID.innerText = "";
        JFVersion.innerText = "";
    }
}

async function main() {
    const data = await APIGet("/api/local");

    if (!data.success || !data.data.online) {
        document.querySelector(".content").innerText = "Dashboard offline";
        return;
    }

    let currentIP = "";
    for (let i = 0; i < data.data.ips.length; ++i) {
        const ip = data.data.ips[i];
        if (ip == "127.0.0.1") continue;
        if (ip == "0.0.0.0") continue;
        currentIP = ip;
        if (ip.startsWith("192.")) break;
    }

    IP.innerText = currentIP;

    updateServerList();
    updateMCServerInfo();
    updateJellyfinInfo();

    // no setInterval for updateServerList since it is self managed
    setInterval(updateMCServerInfo, 10500);
    setInterval(updateJellyfinInfo, 30500);
}

(() => {
    main();
})();
