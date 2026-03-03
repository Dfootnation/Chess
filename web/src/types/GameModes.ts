export type GameMode = "bullet" | "blitz" | "rapid";
export const GAME_TIMES: Record<GameMode, number> = {
	bullet: 60,   // 1  minute
	blitz: 300,   // 5  minutes
    rapid: 600   // 10 minutes
};