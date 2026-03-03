import { useNavigate } from "react-router-dom";

function Home() {
  const navigate = useNavigate();

  return (
    <div style={{ textAlign: "center", marginTop: "100px" }}>
      <h1>Select Game Mode</h1>

      <button onClick={() => navigate("/game/bullet")}>
        Bullet (1 min)
      </button>

      <button onClick={() => navigate("/game/blitz")}>
        Blitz (5 min)
      </button>

      <button onClick={() => navigate("/game/rapid")}>
        Rapid (10 min)
      </button>
    </div>
  );
}

export default Home;