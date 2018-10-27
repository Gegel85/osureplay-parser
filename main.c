#include <stdlib.h>
#include <unistd.h>
#include <osu_replay_parser.h>

int	main(int argc, char **args)
{
	OsuReplay	replay;
	FILE		*stream;
	int		fd;

	if (argc != 2) {
		printf("Usage: %s <replay file>\n", args[0]);
		return EXIT_FAILURE;
	}

	//Load the file
	replay = OsuReplay_parseReplayFile(args[1]);
	if (replay.error) {
		printf("An error occured when reading file\n%s", replay.error);
		return EXIT_FAILURE;
	}

	//Display some info about the replay
	printf("Replay infos:\n");
	printf("Game mode: %s\n", OsuReplay_gameModeToString(replay.mode));
	printf("Game version: %u\n", replay.version);
	printf("Beatmap hash: %s\n", replay.mapHash);
	printf("Player name: %s\n", replay.playerName);
	printf("Replay hash: %s\n", replay.replayHash);
	if (replay.mode == GAME_STD) {
		printf("300: %hu\n", replay.score.nbOf300);
		printf("100: %hu\n", replay.score.nbOf100);
		printf("50: %hu\n", replay.score.nbOf50);
		printf("Miss: %hu\n", replay.score.nbOfMiss);
		printf("Gekis: %hu\n", replay.score.nbOfGekis);
		printf("Katus: %hu\n", replay.score.nbOfKatus);
	} else if (replay.mode == GAME_TAIKO) {
		printf("300: %hu\n", replay.score.nbOf300);
		printf("150: %hu\n", replay.score.nbOf100);
		printf("Miss: %hu\n", replay.score.nbOfMiss);
	} else if (replay.mode == GAME_CTB) {
		printf("300: %hu\n", replay.score.nbOf300);
		printf("100: %hu\n", replay.score.nbOf100);
		printf("Small fruits: %hu\n", replay.score.nbOf50);
		printf("Miss: %hu\n", replay.score.nbOfMiss);
	} else if (replay.mode == GAME_MANIA) {
		printf("Max 300: %hu\n", replay.score.nbOfGekis);
		printf("300: %hu\n", replay.score.nbOf300);
		printf("200: %hu\n", replay.score.nbOf100);
		printf("100: %hu\n", replay.score.nbOfKatus);
		printf("50: %hu\n", replay.score.nbOf50);
		printf("Miss: %hu\n", replay.score.nbOfMiss);
	}
	printf("Max combo: %hu\n", replay.score.maxCombo);
	printf("Total score: %u\n", replay.score.totalScore);
	printf("Did the player combo break ?: %s\n", replay.score.noComboBreak ? "No" : "Yes");
	return EXIT_SUCCESS;
}