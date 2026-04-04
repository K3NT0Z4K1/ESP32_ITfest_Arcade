import { initializeApp }   from "https://www.gstatic.com/firebasejs/10.12.2/firebase-app.js";
import { getDatabase, ref, set, get, onValue, query, orderByChild, limitToLast }
  from "https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";

// ── Firebase config ───────────────────────────────────────────
const firebaseConfig = {
  apiKey:            "AIzaSyDQkY6TuPJpvd5r7ZwMXxlaxxsOTR7l470",
  authDomain:        "dino-game-59371.firebaseapp.com",
  databaseURL:       "https://dino-game-59371-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId:         "dino-game-59371",
  storageBucket:     "dino-game-59371.firebasestorage.app",
  messagingSenderId: "57888134950",
  appId:             "1:57888134950:web:55edb0a5c1b31f957dd3e7",
  measurementId:     "G-V7V6T1NWVW"
};

const app = initializeApp(firebaseConfig);
const db  = getDatabase(app);

// ── Firebase status indicator ─────────────────────────────────
function setFbStatus(ok, msg) {
  const dot   = document.getElementById('fbDot');
  const label = document.getElementById('fbStatusLabel');
  const sub   = document.getElementById('fbStatusSub');
  dot.className   = 'status-dot' + (ok ? '' : ' offline');
  label.textContent = ok ? 'LIVE' : 'OFFLINE';
  sub.textContent   = msg;
}

// ── Games metadata ────────────────────────────────────────────
const GAMES = [
  { id:'dino',  num:'01', name:'Dino Run',    hint:'UP=jump  avoid cacti', icon:'🦕' },
  { id:'snake', num:'02', name:'Snake',       hint:'Joystick = steer',     icon:'🐍' },
  { id:'maze',  num:'03', name:'Maze Runner', hint:'60s timer per level',  icon:'🧩' },
];
let currentTab = 'dino';

// ── Render game cards ─────────────────────────────────────────
function renderGames(hiMap={}) {
  document.getElementById('gamesGrid').innerHTML = GAMES.map(g => `
    <div class="game-card" onclick="switchTab('${g.id}')">
      <div class="game-num">${g.num}</div>
      <div class="game-name">${g.icon} ${g.name}</div>
      <div class="game-hint">${g.hint}</div>
      <div class="game-hi" id="hi-${g.id}">HI: ${hiMap[g.id] || '---'}</div>
    </div>
  `).join('');
}

// ── Tabs ──────────────────────────────────────────────────────
function renderTabs() {
  document.getElementById('lbTabs').innerHTML = GAMES.map(g => `
    <div class="lb-tab ${g.id===currentTab?'active':''}" onclick="switchTab('${g.id}')">${g.icon} ${g.name.split(' ')[0]}</div>
  `).join('');
}

// ── Leaderboard renderer ──────────────────────────────────────
const RANK_EMOJI = ['🥇','🥈','🥉'];
function renderRows(scores) {
  const list = document.getElementById('lbList');
  if (!scores.length) {
    list.innerHTML = '<div class="lb-empty">No scores yet<br>Be the first!</div>';
    return;
  }
  list.innerHTML = scores.map((s,i) => `
    <div class="lb-row" style="animation-delay:${i*0.04}s">
      <div class="lb-rank ${i<3?'r'+(i+1):''}">${i<3?RANK_EMOJI[i]:(i+1)}</div>
      <div class="lb-name ${i===0?'top1':''}">${s.name}</div>
      <div class="lb-score">${s.score}</div>
    </div>
  `).join('');
}

// ── Live leaderboard per game (onValue listener) ──────────────
let lbUnsub = null;

function listenLeaderboard(gameId) {
  if (lbUnsub) { lbUnsub(); lbUnsub = null; }
  document.getElementById('lbList').innerHTML = '<div class="lb-loading">Loading...</div>';

  const q = query(ref(db, `/scores/${gameId}`), orderByChild('score'), limitToLast(10));
  lbUnsub = onValue(q, snap => {
    const rows = [];
    snap.forEach(child => { const v = child.val(); if (v && v.name && v.score !== undefined) rows.push(v); });
    rows.sort((a,b) => b.score - a.score);
    renderRows(rows);

    // Update hi on game card
    if (rows.length > 0) {
      const el = document.getElementById('hi-'+gameId);
      if (el) el.textContent = 'HI: ' + rows[0].score;
    }
  }, err => {
    document.getElementById('lbList').innerHTML = '<div class="lb-empty">Error loading scores</div>';
    setFbStatus(false, err.message);
  });
}

// Expose globally for onclick
window.switchTab = function(id) {
  currentTab = id;
  renderTabs();
  listenLeaderboard(id);
};

// ── LCD preview helper ────────────────────────────────────────
function pad(str, len=20) {
  str = String(str);
  return str.length >= len ? str.substring(0,len) : str + ' '.repeat(len-str.length);
}
function updateLCD(name='') {
  const el = document.getElementById('lcdPreview');
  const rows = name
    ? [pad('* IT FEST ARCADE *'), pad('Hello, '+name+'!'), pad('Hold >> to play'), pad('Good luck! :)')]
    : [pad('* IT FEST ARCADE *'), pad('  3 Games Inside'), pad('  Waiting player...'), pad('')];
  el.innerHTML = rows.map(r => `<span class="lcd-row">${r}</span>`).join('');
}

// ── Register player → write to /currentPlayer ────────────────
window.registerPlayer = async function() {
  const input  = document.getElementById('nameIn');
  const btn    = document.getElementById('startBtn');
  const status = document.getElementById('statusMsg');
  const name   = input.value.trim();

  if (!name || name.length < 2) {
    status.textContent = '⚠ Enter at least 2 characters.';
    status.className   = 'status-msg err';
    return;
  }

  btn.disabled       = true;
  status.textContent = '● Sending to LCD station...';
  status.className   = 'status-msg wait';

  try {
    await set(ref(db, '/currentPlayer'), {
      name,
      ready:     true,
      timestamp: Date.now()
    });
    status.textContent = '✔ Ready! Walk to the LCD station.';
    status.className   = 'status-msg ok';
    document.getElementById('npName').textContent = name.toUpperCase();
    updateLCD(name);
    input.value = '';
    setFbStatus(true, 'Player registered');
    setTimeout(() => {
      btn.disabled = false;
      status.textContent = '';
      status.className   = 'status-msg';
    }, 8000);
  } catch (err) {
    status.textContent = '✖ ' + err.message;
    status.className   = 'status-msg err';
    btn.disabled = false;
    setFbStatus(false, err.message);
  }
};

// Enter key
document.getElementById('nameIn').addEventListener('keydown', e => {
  if (e.key === 'Enter') window.registerPlayer();
});

// ── Live "now playing" from /currentPlayer/name ───────────────
onValue(ref(db, '/currentPlayer/name'), snap => {
  const name = snap.val();
  if (name && name.length >= 2) {
    document.getElementById('npName').textContent = name.toUpperCase();
  } else {
    document.getElementById('npName').textContent = '— Waiting —';
  }
});

// ── Init ──────────────────────────────────────────────────────
renderGames();
renderTabs();
updateLCD();

// Test Firebase connection
try {
  get(ref(db, '/currentPlayer/name')).then(() => {
    setFbStatus(true, 'dino-game-59371 connected');
  }).catch(err => {
    setFbStatus(false, err.message);
  });
} catch(e) {
  setFbStatus(false, e.message);
}

// Start listening to first tab
listenLeaderboard(currentTab);