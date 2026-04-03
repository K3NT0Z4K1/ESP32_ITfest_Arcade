// ── Game metadata ─────────────────────────────────────────────
const GAMES = [
  { id:'dino',   num:'01', name:'Dino Run',    hint:'UP=jump  DOWN=duck',    icon:'🦕', hi:0 },
  { id:'snake',  num:'02', name:'Snake',       hint:'Joystick = steer',      icon:'🐍', hi:0 },
  { id:'maze',   num:'03', name:'Maze Runner', hint:'Navigate to EXIT',      icon:'🧩', hi:0 },
  { id:'flappy', num:'04', name:'Flappy Bird', hint:'UP=flap  avoid pipes',  icon:'🐦', hi:0 },
  { id:'pac',    num:'05', name:'Pac-Man',     hint:'Eat all the dots',      icon:'👻', hi:0 },
  { id:'tetris', num:'06', name:'Tetris',      hint:'X=move UP=rotate',      icon:'🟦', hi:0 },
];

// ── Sample leaderboard data (replace with Firebase later) ──────
const SAMPLE_SCORES = {
  dino:   [ {name:'Angelo',score:312},{name:'Maria',score:287},{name:'Jake',score:201},{name:'Sofia',score:155},{name:'Luis',score:98} ],
  snake:  [ {name:'Reyes',score:24},{name:'Kim',score:19},{name:'Cruz',score:14},{name:'Tan',score:11},{name:'Bautista',score:8} ],
  maze:   [ {name:'Garcia',score:480},{name:'Santos',score:390},{name:'Lim',score:310},{name:'Flores',score:260},{name:'Ramos',score:150} ],
  flappy: [ {name:'Torres',score:18},{name:'Dela Cruz',score:12},{name:'Villanueva',score:9},{name:'Mendoza',score:7},{name:'Reyes',score:5} ],
  pac:    [ {name:'Lopez',score:2340},{name:'Aquino',score:1980},{name:'Vergara',score:1640},{name:'Padilla',score:1200},{name:'Macaraeg',score:890} ],
  tetris: [ {name:'Perez',score:1100},{name:'Domingo',score:870},{name:'Castillo',score:720},{name:'Navarro',score:540},{name:'Ocampo',score:310} ],
};

let currentTab = 'dino';

// ── Render games grid ──────────────────────────────────────────
function renderGames() {
  const grid = document.getElementById('gamesGrid');
  grid.innerHTML = GAMES.map(g => `
    <div class="game-card" onclick="switchTab('${g.id}')">
      <div class="game-num">${g.num}</div>
      <div class="game-name">${g.icon} ${g.name}</div>
      <div class="game-hint">${g.hint}</div>
      <div class="game-hi">HI: ${g.hi}</div>
    </div>
  `).join('');
}

// ── Render leaderboard tabs ────────────────────────────────────
function renderTabs() {
  const tabs = document.getElementById('lbTabs');
  tabs.innerHTML = GAMES.map(g => `
    <div class="lb-tab ${g.id===currentTab?'active':''}" onclick="switchTab('${g.id}')">${g.icon} ${g.name.split(' ')[0]}</div>
  `).join('');
}

// ── Render leaderboard list ────────────────────────────────────
const RANK_EMOJI = ['🥇','🥈','🥉'];
function renderLeaderboard(gameId) {
  const list = document.getElementById('lbList');
  const scores = SAMPLE_SCORES[gameId] || [];
  if (!scores.length) {
    list.innerHTML = '<div class="lb-empty">No scores yet<br>Be the first!</div>';
    return;
  }
  list.innerHTML = scores
    .sort((a,b) => b.score - a.score)
    .map((s, i) => `
      <div class="lb-row" style="animation-delay:${i*0.04}s">
        <div class="lb-rank ${i===0?'r1':i===1?'r2':i===2?'r3':''}">${i<3?RANK_EMOJI[i]:(i+1)}</div>
        <div class="lb-name ${i===0?'top1':''}">${s.name}</div>
        <div class="lb-score">${s.score}</div>
      </div>
    `).join('');
}

function switchTab(id) {
  currentTab = id;
  renderTabs();
  renderLeaderboard(id);
}

// ── LCD preview ────────────────────────────────────────────────
function pad(str, len=20) {
  str = String(str);
  return str.length >= len ? str.substring(0,len) : str + ' '.repeat(len-str.length);
}
function updateLCD(name='') {
  const el = document.getElementById('lcdPreview');
  const rows = name
    ? [ pad('* IT FEST ARCADE *'), pad('Hello, '+name+'!'), pad('Hold >> to play'), pad('Good luck! :)') ]
    : [ pad(''), pad('  IT FEST ARCADE'), pad('  Waiting...'), pad('') ];
  el.innerHTML = rows.map(r => `<div class="lcd-row">${r}</div>`).join('');
}

// ── Register player (placeholder — Firebase connects later) ───
function registerPlayer() {
  const input  = document.getElementById('nameIn');
  const status = document.getElementById('statusMsg');
  const name   = input.value.trim();

  if (!name || name.length < 2) {
    status.textContent = '⚠ Enter at least 2 characters.';
    status.className   = 'status-msg err';
    return;
  }

  status.textContent = '● Sending to station...';
  status.className   = 'status-msg wait';

  // Simulate a brief send delay (replace with Firebase set later)
  setTimeout(() => {
    status.textContent = '✔ Ready! Walk to the LCD station.';
    status.className   = 'status-msg ok';
    document.getElementById('npName').textContent = name.toUpperCase();
    updateLCD(name);
    input.value = '';
    setTimeout(() => {
      status.textContent = '';
      status.className   = 'status-msg';
    }, 7000);
  }, 600);
}

// Enter key support
document.getElementById('nameIn').addEventListener('keydown', e => {
  if (e.key === 'Enter') registerPlayer();
});

// ── Init ──────────────────────────────────────────────────────
renderGames();
renderTabs();
renderLeaderboard(currentTab);
updateLCD();