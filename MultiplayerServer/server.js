const ACTOR_TYPES = require('./Config/ActorType.js');
const serverConfig = require('./Config/ServerConfig.js');

const WebSocket = require('ws');
const http = require('http');

const httpServer = http.createServer((req, res) => {
  if (req.url === '/' || req.url === '/health') {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Server is running');
  } else {
    res.writeHead(404);
    res.end('Not Found');
  }
});

const server = new WebSocket.Server({ 
  noServer: true,
  perMessageDeflate: false
});

httpServer.on('upgrade', (req, socket, head) => {
  if (req.headers['sec-websocket-protocol'] === '') {
    delete req.headers['sec-websocket-protocol'];
  }

  server.handleUpgrade(req, socket, head, (ws) => {
    server.emit('connection', ws, req);
  });
});

httpServer.listen(process.env.PORT || 8080, '0.0.0.0', () => {
  console.log('Server Started on port ' + (process.env.PORT || 8080));
});

const rooms = new Map();
let nextNetId = 1;

function generateNetId() {
  return nextNetId++;
}

function getOrCreateRoom(roomId, maxPlayers = serverConfig.maxPlayers) {
  if (!rooms.has(roomId)) {
    rooms.set(roomId, { 
	  roomId: roomId,
	  maxPlayers: maxPlayers,
      entities: new Map()
    });
  }
  return rooms.get(roomId);
}

server.on('connection', (ws, req) => {
  const origin = req.headers.origin;
  const allowedOrigins = ['https://itch.io', 'https://html-classic.itch.zone', 'http://localhost'];
  //Security check, disable the comment to activate it.
  //if (!allowedOrigins.some(o => origin?.includes(o))) { ws.close(); return; }

  let clientId = null;
  let netId = null;
  let currentRoomId = null;
  let currentRoom = null;
  let isPlayer = false;

  let messageCount = 0;
  let lastReset = Date.now();

  ws.on('message', (raw) => {
    // Rate limit : max 30 msg/s
    const now = Date.now();
    if (now - lastReset > 1000) { messageCount = 0; lastReset = now; }
    messageCount++;
    if (messageCount > 30) return;

    let msg;
    try { msg = JSON.parse(raw); } catch (e) { return; }

    switch (msg.type) {
      
      // =============== Player Management ===============
      case 'JOIN_ROOM': {
		  if (currentRoom && netId !== null && isPlayer) {
			currentRoom.entities.delete(netId);

			currentRoom.entities.forEach((entity) => {
			  if (entity.isRealPlayer && entity.ws && entity.ws.readyState === WebSocket.OPEN) {
				entity.ws.send(JSON.stringify({
				  type: 'PLAYER_LEFT',
				  netId: netId
				}));
			  }
			});

			const hasPlayersLeft = Array.from(currentRoom.entities.values())
			  .some(e => e.isRealPlayer);

			if (!hasPlayersLeft) {
			  rooms.delete(currentRoomId);
			  console.log(`[ROOM] Room ${currentRoomId} deleted (Empty, left on rejoin)`);
			}
		  }

		  clientId = msg.clientId || generateId();
		  currentRoomId = msg.roomId || 'default';

		  const requestedMaxPlayers = msg.maxPlayers || 8;
		  currentRoom = getOrCreateRoom(currentRoomId, requestedMaxPlayers);

		  const currentPlayerCount = Array.from(currentRoom.entities.values())
			.filter(e => e.isRealPlayer === true).length;

		  if (currentPlayerCount >= currentRoom.maxPlayers) {
			ws.send(JSON.stringify({
			  type: 'JOIN_REFUSED',
			  reason: 'ROOM_FULL',
			  roomId: currentRoomId
			}));
			break;
		  }

		  netId = generateNetId();
		  isPlayer = true;

		  const entityData = {
			netId: netId,
			clientId: clientId,
			ws: ws,
			isRealPlayer: true,
			x: 0, y: 0, z: 0,
			rotation: 0
		  };

		  currentRoom.entities.set(netId, entityData);

		  console.log(`[JOIN] Player netId=${netId} (clientId=${clientId}) joined room ${currentRoomId}`);
		  console.log(`[ROOM] Total Entities : ${currentRoom.entities.size}`);

		  const existingActors = Array.from(currentRoom.entities.values())
			.filter(e => e.isRealPlayer !== true)
			.map(e => ({
			  netId: e.netId,
			  actorType: e.actorType,
			  ownerId: e.ownerId,
			  state: e.state || {}
			}));

		  console.log(`[INIT] Sending ${existingActors.length} actors to new client`);
		  console.log(`[INIT] Actors:`, existingActors);

		  ws.send(JSON.stringify({
			type: 'INIT_PLAYERS',
			yourNetId: netId,
			existingActors: existingActors
		  }));

		  ws.send(JSON.stringify({
			type: 'JOINED',
			clientId: clientId,
			netId: netId
		  }));

		  currentRoom.entities.forEach((entity, id) => {
			if (entity.isRealPlayer === true && id !== netId && entity.ws && entity.ws.readyState === WebSocket.OPEN) {
			  entity.ws.send(JSON.stringify({
				type: 'PLAYER_JOINED',
				netId: netId,
				x: 0, y: 0, z: 0,
				rotation: 0
			  }));
			}
		  });
		  break;
		}
		
	  case 'LIST_ROOMS': {
		  const roomList = Array.from(rooms.values()).map(room => {
			const playerCount = Array.from(room.entities.values())
			  .filter(e => e.isRealPlayer === true).length;

			return {
			  roomId: room.roomId,
			  playerCount: playerCount,
			  maxPlayers: room.maxPlayers
			};
		  });

		  ws.send(JSON.stringify({
			type: 'ROOM_LIST',
			rooms: roomList
		  }));
		  break;
		}

      case 'UPDATE_PLAYER_TRANSFORM': {
        if (!currentRoom || netId === null || !isPlayer) return;

        const entity = currentRoom.entities.get(netId);
        if (!entity || !entity.isPlayer) return;

        entity.x = msg.x;
        entity.y = msg.y;
        entity.z = msg.z;
        entity.rotation = msg.rotation;

        currentRoom.entities.forEach((e, id) => {
          if (e.isRealPlayer && id !== netId && e.ws.readyState === WebSocket.OPEN) {
            e.ws.send(JSON.stringify({
              type: 'PLAYER_TRANSFORM_UPDATE',
              netId: netId,
              x: msg.x,
              y: msg.y,
              z: msg.z,
              rotation: msg.rotation
            }));
          }
        });
        break;
      }

      // =============== Actor RPC ===============
      case 'SPAWN': {
        if (!currentRoom) return;
        
        const entityData = {
          netId: msg.netId,
          actorType: msg.actorType || 'UNKNOWN',
          ownerId: clientId,
          state: msg.state,
		  isRealPlayer: false,
		  ws: null
        };
        
        currentRoom.entities.set(msg.netId, entityData);
        console.log(`[SPAWN] room="${currentRoomId}" netId="${msg.netId}" ActorType=${entityData.actorType}`);
		console.log(`[SPAWN] Total entities now: ${currentRoom.entities.size}`);
        
        broadcast(currentRoom, { 
          type: 'SPAWN', 
          netId: msg.netId, 
          state: msg.state, 
          actorType: msg.actorType,
          ownerId: clientId 
        }, ws);
        break;
      }

      case 'UPDATE': {
		  if (!currentRoom) return;
		  const entity = currentRoom.entities.get(msg.netId);

		  if (entity) {
			if (typeof entity.state === 'string') {
			  try { entity.state = JSON.parse(entity.state); } catch(e) { entity.state = {}; }
			}
			if (!entity.state || typeof entity.state !== 'object') {
			  entity.state = {};
			}

			let incomingState = msg.state;
			if (typeof incomingState === 'string') {
			  try { incomingState = JSON.parse(incomingState); } catch(e) { incomingState = {}; }
			}

			Object.assign(entity.state, incomingState);
			
			console.log(`[UPDATE] netId=${msg.netId} new state:`, entity.state);

			broadcast(currentRoom, { 
			  type: 'UPDATE', 
			  netId: msg.netId, 
			  state: msg.state,
			  senderId: clientId 
			}, ws);
		  }
		  break;
		}

      case 'RPC': {
        if (!currentRoom) return;
        broadcast(currentRoom, {
          type: 'RPC',
          netId: msg.netId,
          eventName: msg.eventName,
          payload: msg.payload,
          senderId: clientId
        }, msg.includeSelf ? null : ws);
        break;
      }

      case 'DESPAWN': {
        if (!currentRoom) return;
        currentRoom.entities.delete(msg.netId);
        broadcast(currentRoom, { type: 'DESPAWN', netId: msg.netId }, ws);
        break;
      }

      // =============== Legacy Support ===============
      case 'JOIN': {
        clientId = msg.clientId || generateId();
        currentRoomId = msg.roomId || 'default';
        currentRoom = getOrCreateRoom(currentRoomId);
        
        if (netId === null) {
          netId = generateNetId();
          isPlayer = true;
          const entityData = {
            netId: netId,
            clientId: clientId,
            ws: ws,
            isPlayer: true,
            x: 0, y: 0, z: 0,
            rotation: 0
          };
          currentRoom.entities.set(netId, entityData);
        }

        ws.send(JSON.stringify({ 
          type: 'JOINED', 
          clientId: clientId,
          netId: netId 
        }));

        for (const [entityNetId, entity] of currentRoom.entities) {
          if (!entity.isPlayer) {
            ws.send(JSON.stringify({ 
              type: 'ACTOR_STATE', 
              netId: entityNetId, 
              state: entity.state 
            }));
          }
        }
        break;
      }
    }
  });

  ws.on('close', () => {
  if (currentRoom && netId !== null && isPlayer) {
    
    const actorsToRemove = Array.from(currentRoom.entities.values())
      .filter(e => e.ownerId === clientId && !e.isRealPlayer);
    
    actorsToRemove.forEach(actor => {
      currentRoom.entities.delete(actor.netId);
      broadcast(currentRoom, { 
        type: 'DESPAWN', 
        netId: actor.netId 
      });
    });
    
    currentRoom.entities.delete(netId);
    console.log(`[DISCONNECT] Player netId=${netId} disconnected from ${currentRoomId}`);
    console.log(`[CLEANUP] Removed ${actorsToRemove.length} orphaned actors`);

    currentRoom.entities.forEach((entity) => {
      if (entity.isRealPlayer && entity.ws && entity.ws.readyState === WebSocket.OPEN) {
        entity.ws.send(JSON.stringify({
          type: 'PLAYER_LEFT',
          netId: netId
        }));
      }
    });

    const hasPlayers = Array.from(currentRoom.entities.values())
      .some(e => e.isRealPlayer);

    if (!hasPlayers) {
      rooms.delete(currentRoomId);
      console.log(`[ROOM] Room ${currentRoomId} deleted (Empty)`);
    }
   }
  });
});

function broadcast(room, data, excludeWs = null) {
  const json = JSON.stringify(data);
  
  for (const [id, entity] of room.entities) {
    if (entity.ws && entity.ws !== excludeWs && entity.ws.readyState === WebSocket.OPEN) {
      entity.ws.send(json);
    }
  }
}

function generateId() {
  return Math.random().toString(36).substring(2, 15);
}

console.log('UE4.23 HTML5 from Pimeine - Server Started');