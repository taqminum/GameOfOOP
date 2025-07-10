#include<graphics.h>
#include<string>
#include<vector>

int idx_current_anim = 0;

const int PLAYER_ANIM_NUM = 9;

IMAGE img_player_left[PLAYER_ANIM_NUM];
IMAGE img_player_right[PLAYER_ANIM_NUM];

POINT player_pos = { 500,500 };

const int PLAYER_SPEED = 2;//算是步长了 

const int PLAYER_WIDTH = 80;
const int PLAYER_HEIGHT = 80;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEISHT = 1280;

bool is_move_up = false;
bool is_move_down = false;
bool is_move_left = false;
bool is_move_right = false;

#pragma comment(lib,"MSIMG32.LIB")//链接到这个库，为了图像处理（alphablend）

inline void putimage_alpha(int x, int y, IMAGE* img)//alpha是透明贴图函数
{
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

class Animation
{
public:
	Animation(LPCTSTR path ,int num, int interval)//path是文件路径，num是动画的帧数,interval是帧之间的间隔.构造函数负责load
	{
		interval_ms = interval;

		TCHAR path_file[256];
		for (size_t i = 0;i < num;i++)
		{
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}

	~Animation()
	{
		for (size_t i = 0;i < frame_list.size();i++)
		{
			delete frame_list[i];
		}
	}

	void Play(int x,int y,int delta)//play函数负责动画跑起来
	{
		timer += delta;
		if (timer > interval_ms)
		{
			idx_frame = (idx_frame + 1) % frame_list.size();
			timer = 0;
		}//定时器到时间就重置动画，并且定时器归零

		putimage_alpha(x, y, frame_list[idx_frame]);
	}
private:
	int timer = 0;//动画计时器
	int idx_frame = 0;//动画帧索引
	int interval_ms = 0;
	std::vector<IMAGE*> frame_list;
};

Animation anim_left_player(_T("image/Wizard_left_%d.png"), 9, 100);
Animation anim_right_player(_T("image/Wizard_right_%d.png"), 9, 100);

void DrawPlayer(int delta, int dir_x)
{
	static bool facing_left = false;
	if (dir_x < 0) facing_left = true;
	else if (dir_x > 0) facing_left = false;

	if (facing_left) anim_left_player.Play(player_pos.x, player_pos.y, delta);
	else anim_right_player.Play(player_pos.x, player_pos.y, delta);
}

int main()
{
	initgraph(1280, 1280, 0);
	bool running = true;

	ExMessage msg;
	IMAGE img_background;

	loadimage(&img_background, _T("image/background.png"));//加载背景
	BeginBatchDraw();

	while (running)
	{
		while (peekmessage(&msg))
		{
			if (msg.message == WM_KEYDOWN)
			{
				switch (msg.vkcode)
				{
				case VK_UP:
					is_move_up = true;
					break;
				case VK_DOWN:
					is_move_down = true;
					break;
				case VK_LEFT:
					is_move_left = true;
					break;
				case VK_RIGHT:
					is_move_right = true;
				}
			}
			else if (msg.message = WM_KEYUP)
			{
				switch (msg.vkcode)
				{
				case VK_UP:
					is_move_up = false;
					break;
				case VK_DOWN:
					is_move_down = false;
					break;
				case VK_LEFT:
					is_move_left = false;
					break;
				case VK_RIGHT:
					is_move_right = false;
				}
			}
		}//键盘上下左右控制移动，msg（message类）

		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);//标准化距离 具体作用在下面的函数体
		if(len_dir != 0)
		{
				double normalized_x =dir_x/len_dir;
				double normalized_y =dir_y/len_dir;
				player_pos.x += (int)(PLAYER_SPEED * normalized_x);
				player_pos.y += (int)(PLAYER_SPEED * normalized_y);
		}//标准化，使得斜向速度仍然为PLAYER_SPEED

		if (player_pos.x < 0) player_pos.x = 0;
		if (player_pos.y < 0) player_pos.y = 0;
		if (player_pos.x + PLAYER_WIDTH > WINDOW_WIDTH) player_pos.x = WINDOW_WIDTH - PLAYER_WIDTH;
		if (player_pos.x + PLAYER_WIDTH > WINDOW_WIDTH) player_pos.x = WINDOW_WIDTH - PLAYER_WIDTH;


		cleardevice();

		putimage(0, 0, &img_background);//放置背景

		DrawPlayer(1000/144, is_move_right - is_move_left);

		FlushBatchDraw();

	};

	EndBatchDraw();

	return 0;
}