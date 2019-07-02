// go7e.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

double komi = 6.5;
const int B_SIZE = 9;
const int WIDTH = B_SIZE + 2;
const int BOARD_MAX = WIDTH * WIDTH;

int board[BOARD_MAX] = {
	3,3,3,3,3,3,3,3,3,3,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,0,3,
	3,3,3,3,3,3,3,3,3,3,3
};

int dir4[4] = { +1,-1,+WIDTH,-WIDTH };

int record[1000];
int ko_z;
int all_playouts = 0;

int get_z(int x, int y)
{
	return (y + 1)*WIDTH + (x + 1);	// 0<= x <=8, 0<= y <=8
}
int get81(int z)
{
	if (z == 0) return 0;
	int y = z / WIDTH;
	int x = z - y * WIDTH;	// 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
	return x * 10 + y;        // x*100+y for 19x19
}
int flip_color(int col) {
	return 3 - col;
}

int check_board[BOARD_MAX];

void count_liberty_sub(int tz, int color, int *p_liberty, int *p_stone)
{
	int z, i;

	check_board[tz] = 1;     // search flag
	(*p_stone)++;            // number of stone
	for (i = 0; i < 4; i++) {
		z = tz + dir4[i];
		if (check_board[z]) continue;
		if (board[z] == 0) {
			check_board[z] = 1;
			(*p_liberty)++;      // number of liberty
		}
		if (board[z] == color) count_liberty_sub(z, color, p_liberty, p_stone);
	}
}

void count_liberty(int tz, int *p_liberty, int *p_stone)
{
	int i;
	*p_liberty = *p_stone = 0;
	for (i = 0; i < BOARD_MAX; i++) check_board[i] = 0;
	count_liberty_sub(tz, board[tz], p_liberty, p_stone);
}

void remove(int tz, int color)
{
	int z, i;

	board[tz] = 0;
	for (i = 0; i < 4; i++) {
		z = tz + dir4[i];
		if (board[z] == color) remove(z, color);
	}
}

const int FILL_EYE_ERR = 1;
const int FILL_EYE_OK = 0;

// put stone. success returns 0. in playout, fill_eye_err = 1.
int put(int tz, int color, int fill_eye_err)
{
	if (tz == 0) { ko_z = 0; return 0; }	// pass

	int around[4][3];
	int un_col = flip_color(color);

	// count 4 neighbor's liberty and stones.
	int space = 0;
	int wall = 0;
	int mycol_safe = 0;
	int capture_sum = 0;
	int ko_maybe = 0;
	int i;
	for (i = 0; i < 4; i++) {
		around[i][0] = around[i][1] = around[i][2] = 0;
		int z = tz + dir4[i];
		int c = board[z];	// color
		if (c == 0) space++;
		if (c == 3) wall++;
		if (c == 0 || c == 3) continue;
		int liberty;
		int stone;
		count_liberty(z, &liberty, &stone);
		around[i][0] = liberty;
		around[i][1] = stone;
		around[i][2] = c;
		if (c == un_col && liberty == 1) { capture_sum += stone; ko_maybe = z; }
		if (c == color && liberty >= 2) mycol_safe++;
	}

	if (capture_sum == 0 && space == 0 && mycol_safe == 0) return 1; // suicide
	if (tz == ko_z) return 2; // ko
	if (wall + mycol_safe == 4 && fill_eye_err) return 3; // eye
	if (board[tz] != 0) return 4;

	for (i = 0; i < 4; i++) {
		int d = around[i][0];
		int c = around[i][2];
		if (c == un_col && d == 1 && board[tz + dir4[i]]) {
			remove(tz + dir4[i], un_col);
		}
	}

	board[tz] = color;

	int liberty, stone;
	count_liberty(tz, &liberty, &stone);
	if (capture_sum == 1 && stone == 1 && liberty == 1) ko_z = ko_maybe;
	else ko_z = 0;
	return 0;
}

void print_board()
{
	int x, y;
	const char *str[3] = { ".","X","O" };

	printf("   ");
	for (x = 0; x < B_SIZE; x++) printf("%d", x + 1);
	printf("\n");
	for (y = 0; y < B_SIZE; y++) {
		printf("%2d ", y + 1);
		for (x = 0; x < B_SIZE; x++) {
			printf("%s", str[board[get_z(x, y)]]);
		}
		printf("\n");
	}
}

int count_score(int turn_color)
{
	int score = 0;
	int kind[3];
	kind[0] = kind[1] = kind[2] = 0;
	int x, y, i;
	for (y = 0; y < B_SIZE; y++) for (x = 0; x < B_SIZE; x++) {
		int z = get_z(x, y);
		int c = board[z];
		kind[c]++;
		if (c != 0) continue;
		int mk[4];
		mk[1] = mk[2] = 0;
		for (i = 0; i < 4; i++) mk[board[z + dir4[i]]]++;
		if (mk[1] && mk[2] == 0) score++;
		if (mk[2] && mk[1] == 0) score--;
	}
	score += kind[1] - kind[2];

	double final_score = score - komi;
	int win = 0;
	if (final_score > 0) win = 1;

	if (turn_color == 2) win = -win;
	return win;
}

int playout(int turn_color)
{
	all_playouts++;
	int color = turn_color;
	int previous_z = 0;
	int loop;
	int loop_max = B_SIZE * B_SIZE + 200;	// for triple ko
	for (loop = 0; loop < loop_max; loop++) {
		// all empty points are candidates.
		int empty[BOARD_MAX];
		int empty_num = 0;
		int x, y;
		for (y = 0; y < B_SIZE; y++) for (x = 0; x < B_SIZE; x++) {
			int z = get_z(x, y);
			if (board[z] != 0) continue;
			empty[empty_num] = z;
			empty_num++;
		}
		int z, r = 0;
		for (;;) {
			if (empty_num == 0) {
				z = 0;
			} else {
				r = rand() % empty_num;
				z = empty[r];
			}
			int err = put(z, color, FILL_EYE_ERR);
			if (err == 0) break;
			empty[r] = empty[empty_num - 1];	// err, delete
			empty_num--;
		}
		if (z == 0 && previous_z == 0) break;	// continuous pass
		previous_z = z;
		//		print_board();
		//		printf("loop=%d,z=%d,c=%d,empty_num=%d,ko_z=%d\n",loop,get81(z),color,empty_num,get81(ko_z));
		color = flip_color(color);
	}
	return count_score(turn_color);
}

int select_best_move(int color)
{
	int try_num = 30; // number of playout
	int    best_z = 0;
	double best_value = -100;

	int board_copy[BOARD_MAX];	// keep current board
	memcpy(board_copy, board, sizeof(board));
	int ko_z_copy = ko_z;

	// try all empty point
	int x, y;
	for (y = 0; y < B_SIZE; y++) for (x = 0; x < B_SIZE; x++) {
		int z = get_z(x, y);
		if (board[z] != 0) continue;

		int err = put(z, color, FILL_EYE_ERR);
		if (err != 0) continue;

		int win_sum = 0;
		int i;
		for (i = 0; i < try_num; i++) {
			int board_copy[BOARD_MAX];
			memcpy(board_copy, board, sizeof(board));
			int ko_z_copy = ko_z;

			int win = -playout(flip_color(color));
			win_sum += win;

			memcpy(board, board_copy, sizeof(board));
			ko_z = ko_z_copy;
		}
		double win_rate = (double)win_sum / try_num;
		//		print_board();
		//		printf("z=%d,win=%5.3f\n",get81(z),win_rate);

		if (win_rate > best_value) {
			best_value = win_rate;
			best_z = z;
			printf("best_z=%d,v=%5.3f,try_num=%d\n", get81(best_z), best_value, try_num);
		}

		memcpy(board, board_copy, sizeof(board));  // resume board
		ko_z = ko_z_copy;
	}
	return best_z;
}


// following are for UCT

typedef struct child {
	int z;       // move position
	int games;   // number of played
	double rate; // winrate
	int next;    // next node
} CHILD;

const int CHILD_MAX = B_SIZE * B_SIZE + 1;  // +1 for PASS

typedef struct node {
	int child_num;
	CHILD child[CHILD_MAX];
	int child_games_sum;
} NODE;

const int NODE_MAX = 10000;
NODE node[NODE_MAX];
int node_num = 0;
const int NODE_EMPTY = -1; // no next node
const int ILLEGAL_Z = -1; // illegal move

void add_child(NODE *pN, int z)
{
	int n = pN->child_num;
	pN->child[n].z = z;
	pN->child[n].games = 0;
	pN->child[n].rate = 0;
	pN->child[n].next = NODE_EMPTY;
	pN->child_num++;
}

// create new node. return node index.
int create_node()
{
	if (node_num == NODE_MAX) { printf("node over Err\n"); exit(0); }
	NODE *pN = &node[node_num];
	pN->child_num = 0;
	pN->child_games_sum = 0;

	int x, y;
	for (y = 0; y < B_SIZE; y++) for (x = 0; x < B_SIZE; x++) {
		int z = get_z(x, y);
		if (board[z] != 0) continue;
		add_child(pN, z);
	}
	add_child(pN, 0);  // add PASS

	node_num++;
	return node_num - 1;
}

int select_best_ucb(int node_n)
{
	NODE *pN = &node[node_n];
	int select = -1;
	double max_ucb = -999;
	int i;
	for (i = 0; i < pN->child_num; i++) {
		CHILD *c = &pN->child[i];
		if (c->z == ILLEGAL_Z) continue;
		double ucb = 0;
		if (c->games == 0) {
			ucb = 10000 + rand();  // try once at least
		} else {
			const double C = 0.31; // depend on program
			ucb = c->rate + C * sqrt(log(pN->child_games_sum) / c->games);
		}
		if (ucb > max_ucb) {
			max_ucb = ucb;
			select = i;
		}
	}
	if (select == -1) { printf("Err! select\n"); exit(0); }
	return select;
}

int search_uct(int color, int node_n)
{
	NODE *pN = &node[node_n];
	CHILD *c = NULL;

	for (;;) {
		int select = select_best_ucb(node_n);
		c = &pN->child[select];
		int z = c->z;
		int err = put(z, color, FILL_EYE_ERR);
		if (err == 0) break;
		c->z = ILLEGAL_Z;     // select other move
	}

	int win;
	if (c->games <= 0) {  // playout in first time. <= 10 can reduce node.
		win = -playout(flip_color(color));
	} else {
		if (c->next == NODE_EMPTY) c->next = create_node();
		win = -search_uct(flip_color(color), c->next);
	}

	// update winrate
	c->rate = (c->rate * c->games + win) / (c->games + 1);
	c->games++;
	pN->child_games_sum++;
	return win;
}

int uct_loop = 1000;  // number of uct loop

int select_best_uct(int color)
{
	node_num = 0;
	int next = create_node();

	int i;
	for (i = 0; i < uct_loop; i++) {
		int board_copy[BOARD_MAX];
		memcpy(board_copy, board, sizeof(board));
		int ko_z_copy = ko_z;

		search_uct(color, next);

		memcpy(board, board_copy, sizeof(board));
		ko_z = ko_z_copy;
	}
	int best_i = -1;
	int max = -999;
	NODE *pN = &node[next];
	for (i = 0; i < pN->child_num; i++) {
		CHILD *c = &pN->child[i];
		if (c->games > max) {
			best_i = i;
			max = c->games;
		}
		//  printf("%3d:z=%2d,games=%5d,rate=%.4f\n",i,get81(c->z),c->games,c->rate);
	}
	int ret_z = pN->child[best_i].z;
	printf("z=%2d,rate=%.4f,games=%d,playouts=%d,nodes=%d\n", get81(ret_z), pN->child[best_i].rate, max, all_playouts, node_num);
	return ret_z;
}


int main()
{
	int color = 1;
	int moves = 0;
	srand((unsigned)time(NULL));

	// init board
	int i, x, y;
	for (i = 0; i < BOARD_MAX; i++) board[i] = 3;
	for (y = 0; y < B_SIZE; y++) for (x = 0; x < B_SIZE; x++) board[get_z(x, y)] = 0;

	for (;;) {
		clock_t bt = clock();
		all_playouts = 0;
#if 0	// 0 for UCT search
		int z = select_best_move(color); // pure monte-calro
#else
		int z = select_best_uct(color);  // UCT
#endif
		int err = put(z, color, FILL_EYE_OK);
		if (err != 0) { printf("Err!\n"); exit(0); }
		record[moves] = z;
		moves++;
		print_board();
		printf("play_z = %d,moves=%d,color=%d,all_playouts=%d\n", get81(z), moves, color, all_playouts);
		double t = (double)(clock() + 1 - bt) / CLOCKS_PER_SEC;
		printf("%.1f sec, %.0f playout/sec\n", t, all_playouts / t);
		if (z == 0 && moves > 1 && record[moves - 2] == 0) break;
		if (moves > 300) break;  // too long
		color = flip_color(color);
	}

	// print SGF game record
	printf("(;GM[1]SZ[%d]KM[%.1f]\r\n", B_SIZE, komi);
	for (i = 0; i < moves; i++) {
		int z = record[i];
		int y = z / WIDTH;
		int x = z - y * WIDTH;
		const char *sStone[2] = { "B", "W" };
		printf(";%s", sStone[i & 1]);
		if (z == 0) {
			printf("[]");
		} else {
			printf("[%c%c]", x + 'a' - 1, y + 'a' - 1);
		}
		if (((i + 1) % 10) == 0) printf("\r\n");
	}
	printf("\r\n)\r\n");

	return 0;
}
