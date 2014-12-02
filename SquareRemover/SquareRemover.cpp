#include <string>
#include <iostream>
#include <queue>
#include <time.h>
#define NUM_MOVES 10000
#define RETMOVESIZE 30000
#define MAXNP1 17
#define MAXN 16
#define MAXSQR 15
#define MAX_SQUARES 225 //we have at most (N-1)^2 squares
#define MAXBUFSIZE 1000
#define THRESH 2 //10 //0
#define LOGDEBUG 1

using namespace std;
class Buffer{
public:
	Buffer(int startSeed,unsigned long long int Q, unsigned long long int modulus);
	~Buffer();
	unsigned long long int get(unsigned int index);
private:
	int startSeed;
	unsigned long long int Q;
	unsigned long long int modulus;
	int buf_start;       
	unsigned int buf_start_val; 
	int size;
	//int buf_end;
	//int buf_end_index;
	unsigned long long int val[MAXBUFSIZE];
	void cache(unsigned int i, unsigned long long  int v); 
};

Buffer::Buffer(int startSeed, unsigned long long int Q = 48271, unsigned long long int modulus = 2147483647)
{
	this->startSeed = startSeed;
	this->modulus = modulus;
	this->Q = Q;
	this->buf_start = 0;
	this->buf_start_val = 0;
	this->size = 0;
	this->cache(0, startSeed);
}
Buffer::~Buffer(){};

void Buffer::cache(unsigned int i, unsigned long long int v){
	if (size==0 || size+buf_start_val == i)
	{
		int next = (buf_start + size) % MAXBUFSIZE;
		val[next] = v;
		if (size == MAXBUFSIZE)
		{
			buf_start_val++;
			buf_start = (buf_start+1)%MAXBUFSIZE;
		}
		else
			size++;
	}
	else
		if (LOGDEBUG) cerr  << "BUFFER ERROR: UNCACHABLE!!!" << endl;
	return; //should not happen, error??
}

unsigned long long int Buffer::get(unsigned int i){
	if (i < buf_start_val)
	{
		if (LOGDEBUG) cerr << "BUFFER ERROR: Increase CACHESIZE!!!" << endl;
		return -1;
	}
	else if ((i >= buf_start_val) && i < (size + buf_start_val))
		return val[(buf_start + (i - buf_start_val)) % MAXBUFSIZE];
	else if (i >= size + buf_start_val)
	{
		int tmp = (buf_start + size - 1) % MAXBUFSIZE;
		while (i >= size + buf_start_val)
		{
			unsigned long long int T = val[tmp] * Q;	
			unsigned long long int javab = (T % modulus);
			cache(buf_start_val + size,javab);
			tmp = (tmp + 1) % MAXBUFSIZE;
		}
		return val[tmp];
	}		
	return -1;
}

class Board{
public:
	Board(int colors, string *board, Buffer *buf, int bufIndex);
	Board(const Board& b);
	~Board();
	bool move(int *action, int *gmi, Board &B); //action and good-move-index
	void evaluate();
	int getBoardValue() const; 
	int getScore() const; 
	int last_move[3]; //The move that converted the parent board to this board.
	int gmi[4]; //last_move in the good-move-indexing format
	void InitChanged(bool yn);
	//int last_move_score; //do we want it?
	bool goodMoves[MAXSQR][MAXSQR][4][2];  //true for good moves (that complete) the square.
	//int generation[MAXSQR][MAXSQR][4][4];
	char getBoard(int i, int j);
	void printBoard();
private:
	void Init(int colors, string *board, Buffer *buf, int bufIndex);
	//void removeCompleteSquares();
	bool changed[MAXSQR][MAXSQR];	
	char board[MAXN][MAXN];   //is this faster than C++'s string?
	int boardValue; //a value of board is defined as the number of squares for which goodMoves>0
	int total_score_so_far; //debug
	int N; //size
	int colors; //number of colors
	Buffer *buf;
	int bufIndex; //index used so far from buf.
	/*int mySeed;*/
	void setSqrChanged(int i, int j);
	void setGoodMoveChanged(int i, int j, int d, int p);
	void updateOldGoodMoves(int i, int j);
	friend ostream& operator<<(ostream& o, const Board &B);
};

ostream& operator<<(ostream& o, const Board &B)
{
	return (o << "{xymove:(" << B.last_move[0] << ',' << B.last_move[1] << ',' << B.last_move[2] << "),gmi:[" << B.gmi[0] << ',' << B.gmi[1] << ',' << B.gmi[2] << ',' << B.gmi[3] << "],s:" << B.getScore() << ",v:" << B.getBoardValue() << "}");
}

void Board::printBoard()
{
	if (LOGDEBUG)
	{
		cerr << '[' << endl;
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
				cerr << (int)(board[i][j]);
			cerr << endl;
		}
		cerr << '[' << endl;
	}
}
int Board::getBoardValue() const
{
	return boardValue;
};

int Board::getScore() const
{
	return total_score_so_far;
};

Board::Board(int colors, string *board, Buffer *buf, int bufIndex = 0)
{
	//if (LOGDEBUG) cerr << "Board:c1" << endl;
	Init(colors, board, buf, bufIndex);
	InitChanged(true);
	this->boardValue = 0;
	this->total_score_so_far = 0;

	for (int i = 0; i<N - 1; i++)
	for (int j = 0; j < N - 1; j++)
	for (int k = 0; k < 4; k++)
	for (int d = 0; d < 2; d++)
		this->goodMoves[i][j][k][d] = false;

	this->last_move[0] = -1;
	this->last_move[1] = -1;
	this->last_move[2] = -1;
	this->gmi[0] = -1;
	this->gmi[1] = -1;
	this->gmi[2] = -1;
	this->gmi[3] = -1;

}


Board::Board(const Board &b)
{
	//if (LOGDEBUG) cerr << "Board:c2" << endl;
	string s[MAXN];
	for (int i = 0; i < b.N; i++)
	{
		char tmp[MAXNP1];		
		for (int j = 0; j < b.N; j++)
			tmp[j] = b.board[i][j] + '0';
		tmp[b.N] = '\0';
		s[i] = string(tmp);
	}
	
	Init(b.colors, s, b.buf, b.bufIndex);
	
	this->boardValue = b.boardValue;
	this->total_score_so_far = b.total_score_so_far;

	for (int i=0; i<N-1; i++)
	for (int j = 0; j < N-1;j++)
	for (int k = 0; k < 4;k++)
	for (int d = 0; d < 2; d++)
		this->goodMoves[i][j][k][d] = b.goodMoves[i][j][k][d];

	this->last_move[0] = b.last_move[0];
	this->last_move[1] = b.last_move[1];
	this->last_move[2] = b.last_move[2];
	this->gmi[0] = b.gmi[0];
	this->gmi[1] = b.gmi[1];
	this->gmi[2] = b.gmi[2];
	this->gmi[3] = b.gmi[3];

}


void Board::Init(int colors, string *board, Buffer *buf, int bufIndex=0)
{
	//if (LOGDEBUG) cerr << "Board:Init" << endl;
	this->colors = colors;
	this->buf = buf;
	this->N = board[0].length();
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{			
			//if (LOGDEBUG) cerr << "Board:init(i,j)=(" <<i <<"," << j << ")" << endl;
			this->board[i][j] = board[i][j]-'0';
		}
	}
	this->bufIndex = bufIndex;
}

void Board::InitChanged(bool yn)
{
	for (int i = 0; i < (N - 1); i++)
	{
		for (int j = 0; j < (N - 1); j++)
		{
			changed[i][j] = yn;
		}
	}
}

Board::~Board()
{
	/*
	if (this->mySeed > 0)
		delete buf;
	*/
}

char Board::getBoard(int i, int j)
{
	return board[i][j];
}

void Board::setSqrChanged(int i, int j)
{
	changed[i][j] = true;
	for (int d = 0; d < 4; d++)
	{
		for (int p = 0; p < 2; p++)
		{
			if (goodMoves[i][j][d][p])
			{
				goodMoves[i][j][d][p] = false;
				boardValue--;
			}
		}
	}
}

void Board::setGoodMoveChanged(int i, int j, int d, int p)
{
	//changed[i][j] = true;
	if (goodMoves[i][j][d][p])
	{
		goodMoves[i][j][d][p] = false;
		boardValue--;
	}
}

void Board::updateOldGoodMoves(int i, int j)
{
	if (i > 1 && j > 0)
		setGoodMoveChanged(i - 2, j - 1, 3, 1);
	if (i > 1 && j < N - 1)
		setGoodMoveChanged(i - 2, j, 2, 0);
	if (i<N - 1 && j>1)
		setGoodMoveChanged(i, j - 2, 1, 1);
	if (i>1 && j>1)
		setGoodMoveChanged(i - 1, j - 2, 3, 0);
	if (i < N - 1 && j < N - 2)
		setGoodMoveChanged(i, j + 1, 0, 1);
	if (i>1 && j < N - 2)
		setGoodMoveChanged(i - 1, j + 1, 2, 1);
	if (i < N - 2 && j<N - 1)
		setGoodMoveChanged(i + 1, j, 0, 0);
	if (i<N - 2, j>0)
		setGoodMoveChanged(i + 1, j - 1, 1, 0);
}
bool Board::move(int *action, int *gmi, Board &B)
{
	//parse action
	int i, j, d;
	i = action[0];
	j = action[1];
	d = action[2];

	B.InitChanged(false);

	B.last_move[0] = i;
	B.last_move[1] = j;
	B.last_move[2] = d;
	B.gmi[0] = gmi[0];
	B.gmi[1] = gmi[1];
	B.gmi[2] = gmi[2];
	B.gmi[3] = gmi[3];

	char tmp = board[i][j];

	switch (d){
	case 0: //top
		if (i <= 0)
		{
			B.total_score_so_far = -1;
			return false;
		}
		if (tmp == B.board[i - 1][j])
			return true;

		B.board[i][j] = B.board[i - 1][j]; //i>0
		B.board[i - 1][j] = tmp;
		if (j > 1)
		{
			if (i > 1)
				B.setGoodMoveChanged(i - 2, j - 2, 3, 0);
			B.setGoodMoveChanged(i - 1, j - 2, 1, 1);
			B.setGoodMoveChanged(i - 1, j - 2, 3, 0);
			if (i < N-1)
				B.setGoodMoveChanged(i, j - 2, 1, 1);
		}
		if (j>0)
		{
			if (i>2)
				B.setGoodMoveChanged(i - 3, j - 1,3,1);
			if (i > 1)
				B.setSqrChanged(i - 2, j - 1);
			B.setSqrChanged(i - 1, j - 1);
			if (i < N - 1)
				B.setSqrChanged(i, j - 1);
			if (i < N-2)
				B.setGoodMoveChanged(i + 1, j - 1,1,0);
		}
		if (j<N - 1)
		{
			if (i>2)
				B.setGoodMoveChanged(i - 3, j,2,0);
			if (i >1)
				B.setSqrChanged(i - 2,j);
			B.setSqrChanged(i - 1,j);
			if (i < N - 1)
				B.setSqrChanged(i,j);
			if (i < N - 2)
				B.setGoodMoveChanged(i + 1, j,0,0);
		}
		if (j<N - 2)
		{
			if (i>1)
				B.setGoodMoveChanged(i - 2, j + 1, 2, 1);
			B.setGoodMoveChanged(i - 1, j + 1, 0, 1);
			B.setGoodMoveChanged(i - 1, j + 1, 2, 1);
			if (i<N - 1)
				B.setGoodMoveChanged(i, j + 1, 0, 1);
		}
		break;
	case 1: //right
		if (j > N-2)
		{
			B.total_score_so_far = -1;
			return false;
		}
		if (tmp == B.board[i][j+1])
			return true;

		B.board[i][j] = B.board[i][j + 1];  //j<=N-2
		B.board[i][j + 1] = tmp;
		if (i > 1)
		{
			if (j > 0)
				B.setGoodMoveChanged(i - 2, j - 1, 3, 1);
			B.setGoodMoveChanged(i - 2, j, 2, 0);
			B.setGoodMoveChanged(i - 2, j, 3, 1);
			if (j<N - 2)
				B.setGoodMoveChanged(i - 2, j + 1, 2, 0);
		}
		if (i > 0)
		{
			if (j > 1)
				B.setGoodMoveChanged(i - 1, j - 2, 3, 0);
			if (j > 0)
				B.setSqrChanged(i - 1, j - 1);
			B.setSqrChanged(i - 1, j);
			if (j < N - 2)
				B.setSqrChanged(i - 1,j + 1);
			if (j < N - 3)
				B.setGoodMoveChanged(i - 1, j + 2, 2, 1);
		}
			  
		if (i < N - 1 )
		{
			if (j>1)
				B.setGoodMoveChanged(i, j - 2, 1, 1);
			if (j>0)
				B.setSqrChanged(i,j - 1);
			B.setSqrChanged(i,j);
			if (j<N-2)
				B.setSqrChanged(i,j+1);
			if (j < N - 3)
				B.setGoodMoveChanged(i, j + 2, 0, 1);
		}
		if (i<N - 2)
		{
			if (j>0)
				B.setGoodMoveChanged(i + 1, j - 1, 1, 0);
			B.setGoodMoveChanged(i + 1, j, 0, 0);
			B.setGoodMoveChanged(i + 1, j, 1, 0);
			if (j<N - 2)
				B.setGoodMoveChanged(i + 1, j + 1, 0, 0);
		}
		break;
	case 2: //down
		if (i>N-2)
		{
			B.total_score_so_far = -1;
			return false;
		}
		if (tmp == B.board[i + 1][j])
			return true;

		//i <=N-2
		B.board[i][j] = B.board[i + 1][j];
		B.board[i + 1][j] = tmp;

		if (j > 1)
		{
			if (i > 0)
				B.setGoodMoveChanged(i - 1, j - 2, 3, 0);
			B.setGoodMoveChanged(i, j - 2, 1, 1);
			B.setGoodMoveChanged(i, j - 2, 3, 0);
			if (i<N - 2)
				B.setGoodMoveChanged(i + 1, j - 2, 1, 1);
		}

		if (j > 0)
		{
			if (i > 1)
				B.setGoodMoveChanged(i - 2, j - 1, 3, 1);
			if (i > 0)
				B.setSqrChanged(i - 1,j - 1);
			B.setSqrChanged(i,j - 1);
			if (i < N - 2)
				B.setSqrChanged(i + 1,j - 1);
			if (i < N - 3)
				B.setGoodMoveChanged(i + 2, j-1, 0, 0);
		}
		if (j<N-1)
		{
			if (i>1)
				B.setGoodMoveChanged(i - 2, j, 2, 0);
			if (i>0)
				B.setSqrChanged(i - 1,j);
			B.setSqrChanged(i,j);
			if (i < N - 2)
				B.setSqrChanged(i + 1,j);
			if (i < N - 3)
				B.setGoodMoveChanged(i + 2, j, 0, 0);
		}
		if (j < N - 2)
		{
			if (i>0)
				B.setGoodMoveChanged(i - 1, j + 1, 2, 1);
			B.setGoodMoveChanged(i, j + 1, 0, 1);
			B.setGoodMoveChanged(i, j + 1, 2, 1);
			if (i < N - 2)
				B.setGoodMoveChanged(i + 1, j + 1, 0, 1);
		}
		break;
	case 3: //left
		if (j <= 0)
		{
			B.total_score_so_far = -1;
			return false;
		}
		if (tmp == B.board[i][j-1])
			return true;

		//j>0
		B.board[i][j] = B.board[i][j - 1];
		B.board[i][j - 1] = tmp;
		if (i > 1)
		{
			if (j > 1)
				B.setGoodMoveChanged(i - 2, j - 2, 3, 1);
			B.setGoodMoveChanged(i - 2, j - 1, 2, 0);
			B.setGoodMoveChanged(i - 2, j - 1, 3, 1);
			if (j<N - 1)
				B.setGoodMoveChanged(i - 2, j, 2, 0);
		}

		if (i > 0)
		{
			if (j > 2)
				B.setGoodMoveChanged(i - 1, j - 3, 3, 0);
			if (j > 1)
				B.setSqrChanged(i - 1,j - 2);
			B.setSqrChanged(i - 1,j - 1);
			if (j < N - 1)
				B.setSqrChanged(i-1,j);
			if (j < N - 2)
				B.setGoodMoveChanged(i - 1, j + 1, 2, 1);
		}
		if (i < N - 1)
		{
			if (j>2)
				B.setGoodMoveChanged(i, j - 3, 1, 1);
			if (j >1)
				B.setSqrChanged(i,j - 2);
			B.setSqrChanged(i,j - 1);
			if (j < N - 1)
				B.setSqrChanged(i,j);
			if (j < N - 2)
				B.setGoodMoveChanged(i, j + 1, 0, 1);
		}
		if (i < N - 2)
		{
			if (j < 1)
				B.setGoodMoveChanged(i + 1, j - 2, 1, 0);
			B.setGoodMoveChanged(i + 1, j - 1, 0, 0);
			B.setGoodMoveChanged(i + 1, j - 1, 1, 0);
			if (j < N - 1)
				B.setGoodMoveChanged(i + 1, j, 0, 0);
		}
		break;
	default:
		B.total_score_so_far = -1;   
		return false;
	} //switch
	B.evaluate();
	return true;
}



void Board::evaluate(){
	//find goodMoves and boardValue (based on chabged squares)
	//This function also takes care of removing the complete squares .
	//The very first board should call this right after InitChanged(true)
	// Sequential boards call it after updating changed() as required by move(action).
	
	//we have (N-1)^2 unique squares:
	//bool changed[MAXSQR][MAXSQR];

	/*
	int i = 0;
	int j = 0;
	while ()*/
	for (int i = 0; i < (N - 1); i++)
	{
		for (int j = 0; j < (N - 1); j++)
		{
			if (!changed[i][j])
				continue;
			if (board[i][j] == board[i][j+1]) //1=2
			{
				if (board[i][j] == board[i+1][j]) //1=3
				{
					if (board[i + 1][j] == board[i + 1][j + 1]) //1=2=3=4
					{
						//complete square!
						total_score_so_far++;
						unsigned long long int z;
						char c1 = board[i][j];
						z = buf->get(bufIndex++);
						board[i][j] = z % colors;
						bool c1diff = (c1!=board[i][j]);
						char c2 = board[i][j+1];
						z = buf->get(bufIndex++);
						board[i][j+1] = z % colors;
						bool c2diff = (c2 != board[i][j+1]);
						char c3 = board[i+1][j];
						z = buf->get(bufIndex++);
						board[i+1][j] = z % colors;
						bool c3diff = (c3 != board[i+1][j]);
						char c4 = board[i+1][j+1];
						z = buf->get(bufIndex++);
						board[i+1][j+1] = z % colors;
						bool c4diff = (c4 != board[i+1][j+1]);

						if (LOGDEBUG) cerr << "Removed SQ(" << i << ',' << j << ") and replaced it with the next 4 colors: (" << (int)(board[i][j]) << ',' << (int)(board[i][j + 1] ) << ',' << (int)(board[i + 1][j] ) << ',' << (int) (board[i + 1][j + 1] ) << ") Score=" << total_score_so_far << endl;

						if (c1diff)
							updateOldGoodMoves(i, j);
						if (c2diff)
							updateOldGoodMoves(i, j+1);
						if (c3diff)
							updateOldGoodMoves(i+1, j);
						if (c4diff)
							updateOldGoodMoves(i, j+1);



						changed[i][j] = true;
						if (i > 0)
						{
							if (j>0 && c1diff)
								setSqrChanged(i - 1,j - 1);
							if (c1diff || c2diff)
								setSqrChanged(i - 1,j);
							if (j<N-2 && c2diff)
								setSqrChanged(i - 1,j + 1);
						}
						if (j > 0)
						{
							if (c1diff || c3diff)
								setSqrChanged(i,j - 1);
							if (i<N - 2 && c3diff)
								setSqrChanged(i + 1,j - 1);
						}
						if (i < N - 2)
						{
							if (c3diff || c4diff)
								setSqrChanged(i + 1, j);
							if (j < N - 2 && c4diff)
								setSqrChanged(i + 1,j + 1);
						}
						if (j<N - 2 && (c2diff || c4diff))
							setSqrChanged(i, j + 1);

						if (i > 0)
							i -= 2;
						else
							i -= 1;
						if (j > 0)
							j -= 2;
						else
							j -= 1;
						if (LOGDEBUG) cerr << "evaluate:goBack()" << i << ',' << j << endl;
						continue; //go back!
					} //if 3==4
					else  //1=2=3!4
					{
						if ((j < N - 2) && (board[i + 1][j] == board[i + 1][j + 2])) //switching 4 with its right makes the square complete.
						{
							goodMoves[i][j][3][0] = true;
							boardValue++;
						}
						if ((i < N - 2) && (board[i + 1][j] == board[i + 2][j + 1])) //switching 4 with its down makes the square complete.
						{
							goodMoves[i][j][3][1] = true;
							boardValue++;
						}
					}
				} 
				else //1=2!3
				{
					if (board[i][j + 1] == board[i + 1][j + 1]) //1=2=4!3
					{
						if ((i < N - 2) && (board[i][j] == board[i + 2][j])) //switch 3 with its down
						{
							goodMoves[i][j][2][0] = true;
							boardValue++;
						}
						if ((j>0) && (board[i][j] == board[i + 1][j - 1])) //3 <--> left
						{
							goodMoves[i][j][2][1] = true;
							boardValue++;
						}

					}
				}
			} 
			else //1!2
			{
				if (board[i][j] == board[i + 1][j])  //1=3!2
				{
					if (board[i][j] == board[i + 1][j + 1]) //1=3=4!2
					{
						if ((i > 0) && (board[i][j] == board[i - 1][j + 1])) // 2 <--> top
						{
							goodMoves[i][j][1][0] = true;
							boardValue++;
						}
						if ((j<N-2) &&(board[i][j] == board[i][j + 2])) // 2 <--> right
						{
							goodMoves[i][j][1][1] = true;
							boardValue++;
						}
					}
				}
				else // 1!2 && 1!3
				{
					if (board[i][j + 1] == board[i + 1][j]) // 2=3!1
					{
						if (board[i][j + 1] == board[i + 1][j + 1]) //2=3=4!1
						{
							if ((i > 0) && (board[i + 1][j] == board[i - 1][j])) // 1 <--> top
							{
								goodMoves[i][j][0][0] = true;
								boardValue++;
							}
							if ((j > 0) && (board[i + 1][j] == board[i][j - 1])) // 1 <--> left
							{
								goodMoves[i][j][0][1] = true;
								boardValue++;
							}
						}
					}
				}
			}
			changed[i][j] = false;
		} //for j
	} //for i
};

class BoardCompare
{
public:
	bool operator()(const Board &B1, const Board &B2) const
	{
		return ( (B1.getBoardValue() + 10*B1.getScore()) < (B2.getBoardValue() + 10*B2.getScore()) );
	}
};

class SquareRemover{
public:
	SquareRemover();
	~SquareRemover();
	vector<int> playIt(int colors, vector<string> board, int startSeed);
private:
	int i;
};

SquareRemover::SquareRemover()
{

}

SquareRemover::~SquareRemover()
{

}

void translateMove2goodMoveIndex(int *move, int *GMI)
{
	if (move[2] == 1 || move[2] == 2) //right or down
	{
		GMI[0] = move[0];
	}
}

vector<int> SquareRemover::playIt(int colors, vector<string> board, int startSeed){
	time_t timerb,timere;
	time(&timerb);
	unsigned long long int iiii = 2147483647;
	//unsigned long long int iiii = 8 * 9; // 2147483647ul * 4827ul;
	if (LOGDEBUG) cerr << "iiiixxx=" << iiii*10000 << endl;
	if (LOGDEBUG) cerr << "PlayIt:Started." << endl;
	//int *moves = (int *) malloc(sizeof(int)*RETMOVESIZE);
	vector<int> moves;
	Buffer buf(startSeed);
	if (LOGDEBUG) cerr << "PlayIt:buf created" << endl;
	string S[MAXN];
	for (unsigned int i = 0; i < board[0].length(); i++)
		S[i] = string(board[i]);
	Board B(colors, S, &buf, 0);
	if (LOGDEBUG) cerr << "PlayIt:B created" << endl;
	//B.InitChanged(true);
	B.evaluate();
	bool just_reevaluated = true;
	if (LOGDEBUG) cerr << "PlayIt:Initial Board evaluated:" << B << endl;
	int N = board[0].length();
	if (LOGDEBUG) cerr << "PlayIt:N=" << N << endl;
	bool continue_random_chain = false;
	int random_chain_action[3];
	int prev_move[3];
	for (int round = 0; round < NUM_MOVES; round++)
	{
		if (LOGDEBUG) cerr << "------\nPlayIt:round=" << round+1 << endl;
		//evaluate as many actions as you can, choose the action which leads to the highest board value + score.
		priority_queue<Board, vector<Board>, BoardCompare> PQ;
		//PQ = priority_queue<Board, vector<Board>, BoardCompare>();

		if (!PQ.empty()) //remove boards corresponding to outdated goodMoves;
		{
			Board B1 = PQ.top();
			while (!B.goodMoves[B1.gmi[0]][B1.gmi[1]][B1.gmi[2]][B1.gmi[3]] && !PQ.empty())
			{
				if (LOGDEBUG) cerr << "PlayIt:outdated board " << B1 << " is being popped.";
				//delete (&B1); 
				PQ.pop();
				if (LOGDEBUG) cerr << " Size(PQ)=" << PQ.size()<< endl;
				B1 = PQ.top();
			}
		}
		
		int action[3];
		int gmi[4];		
		if (PQ.empty())
		{
			//add "some" actions (for speed):			
			int cnt = 0;
			for (int i = 0; i < N - 1; i++)
			{
				gmi[0] = i;
				for (int j = 0; j < N - 1; j++)
				{
					gmi[1] = j;
					for (int d = 0; d < 4; d++)
					{
						gmi[2] = d;
						switch (d)
						{
						case 0:
						{
								  action[0] = i;
								  action[1] = j;
								  if (B.goodMoves[i][j][d][0])
								  {
								  action[2] = 0;
								  gmi[3] = 0;
								  Board *bp = new Board(B);
								  if (B.move(action, gmi, *bp))
								  {
									  PQ.push(*bp);
									  if (LOGDEBUG) cerr << "PlayIt:pushed " << *bp << ". Size(PQ)=" << PQ.size() << endl;
									  cnt++;
								  }
								  }
								  if (B.goodMoves[i][j][d][1])
								  {
								  action[2] = 3;
								  gmi[3] = 1;
								  Board *bp2 = new Board(B);
								  if (B.move(action, gmi, *bp2))
								  {
									  PQ.push(*bp2);
									  if (LOGDEBUG) cerr << "PlayIt:pushed " << *bp2 << ". Size(PQ)=" << PQ.size() << endl;
									  cnt++;
								  }
								  }
								  break;
						}
						case 1:
						{
								  action[0] = i;
								  action[1] = j + 1;
								  if (B.goodMoves[i][j][d][0])
								  {
								  action[2] = 0;
								  gmi[3] = 0;
								  Board *bp3 = new Board(B);
								  if (B.move(action, gmi, *bp3))
								  {
									  PQ.push(*bp3);
									  if (LOGDEBUG) cerr << "PlayIt:pushed " << *bp3 << ". Size(PQ)=" << PQ.size() << endl;
									  cnt++;
								  }
								  }
								  if (B.goodMoves[i][j][d][1])
								  {
								  action[2] = 1;
								  gmi[3] = 1;
								  Board *bp4 = new Board(B);
								  if (B.move(action, gmi, *bp4))
								  {
									  PQ.push(*bp4);
									  if (LOGDEBUG) cerr << "PlayIt:pushed " << *bp4 << ". Size(PQ)=" << PQ.size() << endl;
									  cnt++;
								  }
								  }
								  break;
						}
						case 2:
						{
								  action[0] = i + 1;
								  action[1] = j;
								  if (B.goodMoves[i][j][d][0])
								  {
								  action[2] = 2;
								  gmi[3] = 0;
								  Board *bp5 = new Board(B);
								  if (B.move(action, gmi, *bp5))
								  {
									  PQ.push(*bp5);
									  if (LOGDEBUG) cerr << "PlayIt:pushed " << *bp5 << ". Size(PQ)=" << PQ.size() << endl;
									  cnt++;
								  }
								  }
								  if (B.goodMoves[i][j][d][1])
								  {
								  action[2] = 3;
								  gmi[3] = 1;
								  Board *bp6 = new Board(B);
								  if (B.move(action, gmi, *bp6))
								  {
									  PQ.push(*bp6);
									  if (LOGDEBUG) cerr << "PlayIt:pushed " << *bp6 << ". Size(PQ)=" << PQ.size() << endl;
									  cnt++;
								  }
								  }

								  break;
						}
						case 3:
						{
								  action[0] = i + 1;
								  action[1] = j + 1;
								  if (B.goodMoves[i][j][d][0])
								  {
								  action[2] = 1;
								  gmi[3] = 0;
								  Board *bp7 = new Board(B);
								  if (B.move(action, gmi, *bp7))
								  {
									  PQ.push(*bp7);
									  if (LOGDEBUG) cerr << "PlayIt:pushed " << *bp7 << ". Size(PQ)=" << PQ.size() << endl;
									  cnt++;
								  }
								  }
								  if (B.goodMoves[i][j][d][1])
								  {
								  action[2] = 2;
								  gmi[3] = 1;
								  Board *bp8 = new Board(B);
								  if (B.move(action, gmi, *bp8))
								  {
									  PQ.push(*bp8);
									  if (LOGDEBUG) cerr << "PlayIt:pushed " << *bp8 << ". Size(PQ)=" << PQ.size() << endl;
									  cnt++;
								  }
								  }
								  break;
						}
						} //switch
						if (round > 0 && cnt > THRESH)
							break;
					}
					if (round > 0 && cnt > THRESH)
						break;
				}
				if (round > 0 && cnt > THRESH)
					break;
			}
		}
		
		if (PQ.empty()) //if still empty --> no goodMoves found
		{
			if (false)
			{
				//if (LOGDEBUG) cerr << "PlayIt:Random!" << endl;
				action[0] = rand() % (N - 1);
				action[1] = rand() % (N - 1);


				if (action[0] > 0)
				{
					gmi[0] = action[0] - 1;
					if (action[1] > 0)
					{
						gmi[1] = action[1] - 1;
						gmi[2] = 3;
						action[2] = 2; // a random "down" action
						gmi[3] = 1;
					}
					else
					{
						gmi[1] = action[1]; //0
						gmi[2] = 2;
						action[2] = 2; // a random "down" action
						gmi[3] = 0;
					}
				}
				else
				{
					gmi[0] = action[0]; //0
					if (action[1] > 0)
					{
						gmi[1] = action[1] - 1;
						gmi[2] = 1;
						action[2] = 1; // a random "right" action
						gmi[3] = 1;
					}
					else
					{
						action[1] = 1; //change 0,0 to 0,1
						gmi[1] = action[1];
						gmi[2] = 1;
						action[2] = 1; // a random "right" action
						gmi[3] = 1;
					}
				}
			}
			else 
			{
				if (!continue_random_chain)
				{
					action[0] = (rand() % (N - 2)) + 1;
					action[1] = (rand() % (N - 2)) + 1;
					action[2] = B.getBoard(action[0], action[1]) % 4;  // push each colors to its own side
					continue_random_chain = true;
				}
				else
				{
					action[0] = random_chain_action[0];
					action[1] = random_chain_action[1];
					action[2] = random_chain_action[2];
				}

				//for next round

				switch (action[2])
				{
				case 0:
					gmi[0] = action[0];
					gmi[1] = action[1];
					gmi[2] = 0;
					gmi[3] = 0;
					if (action[0] == 1)
						continue_random_chain = false;
					else
					{
						random_chain_action[0] = action[0] - 1;
						random_chain_action[1] = action[1];
						random_chain_action[2] = action[2];
					}
					break;
				case 1:
					gmi[0] = action[0];
					gmi[1] = action[1] - 1;
					gmi[2] = 1;
					gmi[3] = 1;
					if (action[1] == N-2)
						continue_random_chain = false;
					else
					{
						random_chain_action[0] = action[0];
						random_chain_action[1] = action[1]+1;
						random_chain_action[2] = action[2];
					}
					break;
				case 2:
					gmi[0] = action[0] - 1;
					gmi[1] = action[1] - 1;
					gmi[2] = 3;
					gmi[3] = 1;
					if (action[0] == N-2)
						continue_random_chain = false;
					else
					{
						random_chain_action[0] = action[0] + 1;
						random_chain_action[1] = action[1];
						random_chain_action[2] = action[2];
					}

				case 3:
					gmi[0] = action[0];
					gmi[1] = action[1] - 1;
					gmi[2] = 2;
					gmi[3] = 0;
					if (action[1] == 1)
						continue_random_chain = false;
					else
					{
						random_chain_action[0] = action[0];
						random_chain_action[1] = action[1]-1;
						random_chain_action[2] = action[2];
					}
					break;
				}				
			}
			if (B.move(action, gmi, B))
			{
				if (LOGDEBUG) cerr << "PlayIt:random move: " << B << ". Size(PQ)=" << PQ.size() << endl;
			}
			else
			{
				if (LOGDEBUG) cerr << "PlayIt:THIS SHOULDN't HAPPEN: Random Move Unsuccessful?" << endl;
			}
		}
		else
		{
			continue_random_chain = false;
			B = PQ.top();
			if (LOGDEBUG) cerr << "PlayIt:Played to board " << B << "." << endl;
			B.printBoard();
			PQ.pop();
		}
		/*moves[round * 3]   = B.last_move[0];
		moves[round * 3+1] = B.last_move[1];
		moves[round * 3+2] = B.last_move[2];*/
		moves.push_back(B.last_move[0]);
		moves.push_back(B.last_move[1]);
		moves.push_back(B.last_move[2]);

		//::
		if (!just_reevaluated && (B.last_move[0] == prev_move[0]) && (B.last_move[1] == prev_move[1]) && (B.last_move[2] == prev_move[2]))
		{
			B.goodMoves[B.gmi[0]][B.gmi[1]][B.gmi[2]][B.gmi[3]] = false;
			B.InitChanged(true);
			B.evaluate();
			just_reevaluated = true;
		}
		else
			just_reevaluated = false;
		prev_move[0] = B.last_move[0];
		prev_move[1] = B.last_move[1];
		prev_move[2] = B.last_move[2];

	}

	time(&timere);
	if (LOGDEBUG) cerr << "Done in " << difftime(timere,timerb) << "seconds." << endl;
	return moves;
}

int main(int argc, char*argv[]){
	//test buffer:
	/*Buffer ff(3);
	for (int i = 0; i < 10; i++)
		cout << ff.get(i) << endl;
	return 0;
	*/
	if (LOGDEBUG) cerr << sizeof(int) << endl;
	SquareRemover SQ;
	vector<string> SB;
	vector<int> M;
	int exno = -10; // -10000;
	if (LOGDEBUG) cerr << "Start playIt example" << exno << "..." << endl;
	switch (exno){
	case -1:
		//Example -1:
		SB.push_back(string("000"));
		SB.push_back(string("011"));
		SB.push_back(string("011"));
		M= SQ.playIt(2, SB, 1);
		break;
	case 0:
		//Example 0:
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00104410123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00103310123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("11102010123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00100010123401"));
		SB.push_back(string("00100010123401"));
		M = SQ.playIt(5, SB, 257017653);
		break;
	default:
		int colors;
		int N;
		int startSeed;
		cin >> colors;
		if (LOGDEBUG) cerr << "colors: " << colors << endl;
		cin >> N;
		if (LOGDEBUG) cerr << "N: " << N << endl;
		string ss;
		for (int i = 0; i < N; i++)
		{
			cin >> ss;
			SB.push_back(ss);
		}
		cin >> startSeed;
		if (LOGDEBUG) cerr << "startSeed: " << startSeed << endl;
		M = SQ.playIt(colors, SB, startSeed);
	} //switch
	for (int i = 0; i < NUM_MOVES * 3; i++){
		cout << M[i] << endl;
		if ((i%3)==0)
			if (LOGDEBUG) cerr << i/3 << ":" << M[i] << ',' << M[i+1] <<','  << M[i+2] << endl;
	}
	cout.flush();
	return 0;
};
