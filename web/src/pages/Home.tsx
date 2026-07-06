import { useNavigate } from "react-router-dom";
import { Chess } from "chess.js";
import { ENGINE_URL } from "../types/Constants";

function Home() {
  const navigate = useNavigate();

  async function startGame(mode: string) {
    await fetch(`${ENGINE_URL}/position`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ 
        fen: new Chess().fen() 
      }),
    });
    navigate(`/game/${mode}`);
  }

  return (
    <div style={{ textAlign: "center", marginTop: "100px" }}>
      <h1>Select Game Mode</h1>

      <button onClick={() => startGame("bullet")}>
        Bullet (1 min)
      </button>

      <button onClick={() => startGame("blitz")}>
        Blitz (5 min)
      </button>

      <button onClick={() => startGame("rapid")}>
        Rapid (10 min)
      </button>
    </div>
  );
}

export default Home;
