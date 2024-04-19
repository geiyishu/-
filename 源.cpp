/*
	开发日志
*/
#include<stdio.h>
#include<graphics.h>
#include<time.h>
#include"tools.h"

#include<mmsystem.h>
#pragma comment(lib,"winmm.lib")
//导入音效

#define WIN_WIDTH 900//宽度
#define WIN_HEIGHT 600//长度
enum{WAN_DOU,XIANG_RI_KUI,ZHI_WU_COUNT};//植物类型，数量

IMAGE imgBg;//表示背景图片
IMAGE imgBar;//表示植物框
IMAGE imgCards[ZHI_WU_COUNT];//植物卡牌
IMAGE* imgZhiWu[ZHI_WU_COUNT][20];//植物

int curX, curY;//当前选中的植物，在移动过程中的位置
int curZhiWu;//0:没有选中  1：选中第一种植物

struct zhiwu
{
	int type;         //0:没有植物   1：选择第一种植物
	int frameIndex;   //序列帧的序号
};
struct zhiwu map[3][9];

struct sunshineBall   //定义阳光球
{
	int x, y;         //飘落过程中的坐标
	int frameIndex;   //当前显示的图片帧的序号
	int destY;        //飘落目标的Y坐标
	bool used;         //是否在使用
	int timer;			//计时器
};

struct sunshineBall balls[10];//阳光池
IMAGE imgSunshineBall[29];
int sunshine;

struct zm
{
	int x, y;
	int frameIndex;
	bool used;
	int speed;
	int row;
};
struct zm zms[10];
IMAGE imgZM[22];

//子弹的数据类型
struct bullet
{
	int x, y;
	int row;
	bool used;
	int speed;
};
struct bullet bullets[30];
IMAGE imgBulletNormal;

bool fileExist(const char* name)
{
	FILE* fp = fopen(name, "r");
	if (fp==NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;
	}
}

void gameInit()
{
	//加载有背景图片
	loadimage(&imgBg, "res/bg.jpg");
	loadimage(&imgBar, "res/bar5.png");

	memset(imgZhiWu, 0, sizeof(imgZhiWu));
	memset(map, 0, sizeof(map));

	//初始化植物卡牌
	char name[64];
	for (int i = 0; i < ZHI_WU_COUNT; i++)
	{
		//生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png",i+1);
		loadimage(&imgCards[i], name);

		for (int j = 0; j < 20; j++)
		{
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png",i, j + 1);
			
			//先判断文件是否存在
			if (fileExist(name))
			{
				imgZhiWu[i][j] = new IMAGE;
				loadimage(imgZhiWu[i][j],name);//加载到
			}
			else
			{
				break;
			}
		}
	}

	curZhiWu = 0;
	sunshine = 50;

	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunshineBall[i], name);
	}

	//配置随机种子
	srand((int)time(0));

	//创建游戏图形窗口
	initgraph(WIN_WIDTH,WIN_HEIGHT,1);

	//设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;         //抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT);
	setcolor(BLACK);

	//初始化僵尸数据
	memset(zms, 0, sizeof(zms));
	for (int  i = 0; i < 22; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZM[i], name);
	}

	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));
}

void drawZM()
{
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmCount; i++)
	{
		if (zms[i].used)
		{
			IMAGE* img = &imgZM[zms[i].frameIndex];
			putimagePNG(
				zms[i].x,
				zms[i].y - img->getheight(),
				img);
		}
	}
}

//渲染图片
void updateWindow()
{
	BeginBatchDraw();//开始缓冲

	putimage(0, 0, &imgBg);
	putimagePNG(250, 0, &imgBar);

	for (int i = 0; i < ZHI_WU_COUNT; i++)
	{
		int x = 338 + i * 65;
		int y = 6;
		putimage(x, y, &imgCards[i]);
	}

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type > 0)
			{
				int x = 256 + j * 81;
				int y = 179 + i * 102;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				putimagePNG(x, y, imgZhiWu[zhiWuType][index]);
			}
		}

	}

	//  渲染  拖动过程中的植物
	if (curZhiWu > 0)
	{
		IMAGE* img = imgZhiWu[curZhiWu - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getwidth() / 2, img);
	}

	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		if (balls[i].used)
		{
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].x, balls[i].y, img);
		}
	}
	
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(276, 67, scoreText);//输出分数

	drawZM();

	EndBatchDraw();//结束双缓冲
}

//收集阳光
void collectSunshine(ExMessage* msg)
{
	int count = sizeof(balls) / sizeof(balls[0]);
	int w = imgSunshineBall[0].getwidth();
	int h = imgSunshineBall[0].getheight();
	for (int i = 0; i < count; i++)
	{
		int x = balls[i].x;
		int y = balls[i].y;
		if (balls[i].used)
		{
			if (msg->x > x && msg->x < x + w &&
				msg->y > y && msg->y < y + h)
			{
				balls[i].used = false;
				sunshine += 25;
				mciSendString("play res/sunshine.mp3", 0, 0, 0);
			}

		}
	}
}

//用户点击
void userClick()
{
	ExMessage msg;//消息类型
	static int status = 0;//状态变量
	if (peekmessage(&msg))//判断鼠标有没有消息
	{
		if (msg.message == WM_LBUTTONDOWN)//如果消息是左键
		{
			if (msg.x > 338 && msg.x < 338 + 65 * ZHI_WU_COUNT&& msg.y < 96)
			{
				int index = (msg.x - 338) / 65;
				status = 1;
				curZhiWu = index + 1;

				curX = msg.x;
				curY = msg.y;
			}
			else 
			{
				collectSunshine(&msg);//收集阳光
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1)//如果鼠标移动
		{
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP)//左键抬起来
		{
			if (msg.x > 256 && msg.y > 179 && msg.y < 489)
			{
				int row = (msg.y - 179) / 102;//第几行
				int col = (msg.x - 256) / 81;//第几列

				if (map[row][col].type == 0)
				{
					map[row][col].type = curZhiWu;
					map[row][col].frameIndex = 0;
				}
			}
			
			//种植物
			curZhiWu = 0;
			status = 0;
		}
	}
}

void createSunshine()//创建阳光
{
	static int count = 0;
	static int fre = 400;

	count++;
	if (count >= fre)
	{
		fre = 200 + rand() % 200;
		count = 0;

		//从阳光池中取一个可以使用的
		int ballMax = sizeof(balls) / sizeof(balls[0]);

		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);

		if (i >= ballMax)
		{
			return;
		}

		balls[i].used = true;
		balls[i].frameIndex = 0;
		balls[i].x = 260 + rand() % (900 - 260);      //260--900 //rand()随机数 
		balls[i].y = 60;
		balls[i].destY = 200 + (rand() % 4) * 90;
		balls[i]. timer = 0;
	}
}

void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		if (balls[i].used)
		{
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			balls[i].y += 2;
			if (balls[i].timer == 0)
			{
				balls[i].y += 2;
			}
			if (balls[i].y >= balls[i].destY)
			{
				balls[i].timer++;
				if (balls[i].timer > 100)
				{
					balls[i].used = false;
				}
			}
		}
	}
}//更新阳光状态

void createZM()
{
	static int zmFre = 200;
	static int count = 0;
	count++;
	if (count > zmFre)
	{
		count = 0;
		zmFre = rand() % 200 + 300;

		int i;
		int zmMax = sizeof(zms) / sizeof(zms[0]);
		for (i = 0; i < zmMax && zms[i].used; i++);
		if (i < zmMax)
		{
			zms[i].used = true;
			zms[i].x = WIN_WIDTH;
			zms[i].row = rand() % 3;
			zms[i].y = 172 + (1 + zms[i].row) * 100;
			zms[i].speed = 1;
		}
	}
}

//僵尸状态
void updateZM()
{
	int zmMax = sizeof(zms) / sizeof(zms[0]);

	static int count = 0;
	count++;
	if (count > 2)
	{
		count = 0;
		//更新僵尸的位置
		for (int i = 0; i < zmMax; i++)
		{
			if (zms[i].used)
			{
				zms[i].x -= zms[i].speed;
				if (zms[i].x < 170)
				{
					printf("GAME OVER\n");
					MessageBox(NULL, "over", "over", 0);//待优化
					exit(0);
				}
			}
		}
	}
	static int count2 = 0;
	count2++;
	if (count2 > 4)
	{
		count2 = 0;
		for (int i = 1; i < zmMax; i++)
		{
			if (zms[i].used)
			{
				zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;
			}
		}
	}	
}

void shoot()
{
	int lines[3] = { 0 };
	

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == WAN_DOU + 1)
			{

			}
		}
	}
}

void updateGame()
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type>0)
			{
				map[i][j].frameIndex++;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (imgZhiWu[zhiWuType][index] == NULL)
				{
					map[i][j].frameIndex = 0;
				}
			}
		}
	}

	createSunshine();//创建阳光
	updateSunshine();//更新阳光的状态

	createZM();//创建僵尸
	updateZM();//更新僵尸的状态

	shoot();//发射豌豆子弹
}



//启动界面
void startUI()
{
	IMAGE imgBg, imgMenu1, imgMenu2;
	loadimage(&imgBg, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");

	int flag = 0;

	while (1)
	{
		BeginBatchDraw();//双缓冲

		putimage(0, 0, &imgBg);
		putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);

		

		ExMessage msg;//定义一个消息
		if (peekmessage(&msg))//有消息
		{
			if (msg.message == WM_LBUTTONDOWN &&
				msg.x > 474 && msg.x < 474 + 331 &&
				msg.y > 75 && msg.y <75 + 140
				)//按下
			{
				flag = 1;
				EndBatchDraw();//结束双缓冲
			}
			else if (msg.message == WM_LBUTTONUP && flag == 1)
			{
				return;
			}
		}
		EndBatchDraw();//结束双缓冲
	}

}

int main()
{
	gameInit();

	startUI();

	int timer = 0;//时间统计的变量
	bool flag = true;
	while (true)
	{
		userClick();//接受用户的点击
		timer += getDelay();
		if (timer > 70)
		{
			flag = true;
			timer = 0;
		}

		if (flag)
		{
			flag = false;
			updateWindow();
			updateGame();
		}
	}

	system("pause");
	return 0;
}