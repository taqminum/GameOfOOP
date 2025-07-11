#include<graphics.h>
#include<string>
#include<vector>
#include <cmath>


const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 1280;

#pragma comment(lib,"MSIMG32.LIB")//链接到这个库，为了图像处理（alphablend）
#pragma comment(lib,"Winmm.lib")//链接，为了音乐处理

inline void putimage_alpha(int x, int y, IMAGE* img, BYTE alpha = 255)
{
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };
	AlphaBlend(GetImageHDC(NULL), x, y, img->getwidth(), img->getheight(),
		GetImageHDC(img), 0, 0, img->getwidth(), img->getheight(), bf);
}

class Animation
{
public:
	Animation(LPCTSTR path, int num, int interval)//path是文件路径，num是动画的帧数,interval是帧之间的间隔.构造函数负责load
	{
		interval_ms = interval;

		TCHAR path_file[256];
		for (size_t i = 1;i <= num;i++)
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

	void Play(int x, int y, int delta)
	{
		timer += delta;

		bool isLastFrame = (idx_frame == frame_list.size() - 1);
		int currentInterval = isLastFrame ? interval_ms * 3 / 2 : interval_ms;

		// 计算进度百分比
		float progress = static_cast<float>(timer) / currentInterval;

		if (progress >= 1.0f)
		{
			idx_frame = (idx_frame + 1) % frame_list.size();
			timer = 0;
			progress = 0.0f;
		}

		// 添加帧过渡效果（最后20%时间）
		if (progress > 0.8f)
		{
			int nextFrame = (idx_frame + 1) % frame_list.size();
			float blendFactor = (progress - 0.8f) / 0.2f;

			// 绘制当前帧（逐渐透明）
			putimage_alpha(x, y, frame_list[idx_frame], static_cast<BYTE>(255 * (1 - blendFactor)));

			// 绘制下一帧（逐渐显现）
			putimage_alpha(x, y, frame_list[nextFrame], static_cast<BYTE>(255 * blendFactor));
		}
		else
		{
			putimage_alpha(x, y, frame_list[idx_frame]);
		}
	}

private:
	int timer = 0;//动画计时器
	int idx_frame = 0;//动画帧索引
	int interval_ms = 0;
	std::vector<IMAGE*> frame_list;
};


class Player
{
	public:
		Player()
		{
			anim_left = new Animation(_T("image/Wizard_left_%d.png"), 9, 35);
			anim_right = new Animation(_T("image/Wizard_right_%d.png"), 9, 35);

		}
		~Player()
		{
			delete anim_left;
			delete anim_right;
		}
		void ProcessEvent(const ExMessage& msg)
		{
			switch (msg.message)
			{
			case WM_KEYDOWN:
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
					break;
				}
				break;
			case WM_KEYUP:

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
					break;
				}
				break;
			}
		}
			void Move()
			{ 
				int dir_x = is_move_right - is_move_left;
				int dir_y = is_move_down - is_move_up;
				double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);//标准化距离 具体作用在下面的函数体
				if (len_dir != 0)
				{
					double normalized_x = dir_x / len_dir;
					double normalized_y = dir_y / len_dir;
					position.x += (int)(SPEED * normalized_x);
					position.y += (int)(SPEED * normalized_y);
				}//标准化，使得斜向速度仍然为PLAYER_SPEED

				if (position.x < 0) position.x = 0;
				if (position.y < 0) position.y = 0;
				if (position.x + PLAYER_WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - PLAYER_WIDTH;
				if (position.x + PLAYER_WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - PLAYER_WIDTH;
			}
			void Draw(int delta)
			{
				static bool facing_left = false;
				int dir_x = is_move_right - is_move_left;

				if (dir_x < 0) facing_left = true;
				else if (dir_x > 0) facing_left = false;

				if (facing_left) anim_left->Play(position.x, position.y, delta);
				else anim_right->Play(position.x,position.y, delta);

			}
			const POINT& GetPosition() const
			{
				return position;
			}
		private:

			Animation* anim_left;
			Animation* anim_right;
			POINT position = { 500,500 };
			bool is_move_up = false;
			bool is_move_down = false;
			bool is_move_left = false;
			bool is_move_right = false;
			const int SPEED = 3;//算是步长了 
		public:
			const int PLAYER_WIDTH = 80;
			const int PLAYER_HEIGHT = 80;
};
class Bullet//子弹，武器
{
	public:
		POINT position = { 0,0 };

	public:
		Bullet() = default;
		~Bullet() = default;

		void Draw() const
		{
			setlinecolor(RGB(255, 155, 50));
			setfillcolor(RGB(200, 75, 10));
			fillcircle(position.x, position.y, RADIUS);
		}
	private:
		const int RADIUS = 10;
};
class Enemy
{
	public:
		Enemy()
		{
			anim_left = new Animation(_T("image/actor1_left_%d.png"), 4, 45);
			anim_right = new Animation(_T("image/actor1_right_%d.png"), 4, 45);

			//敌人生成边界
			enum class EmergeEDGE
			{
				Up = 0, Down, Left, Right
			};

			//敌人生成在地图边界处随机位置
			EmergeEDGE edge = (EmergeEDGE)(rand() % 4);
			switch (edge)
			{
			case EmergeEDGE::Up:
				position.x = rand() % WINDOW_WIDTH;
				position.y = -ENEMY_HEIGHT;
				break;
			case EmergeEDGE::Down:
				position.x = rand() % WINDOW_WIDTH;
				position.y = WINDOW_HEIGHT;
				break;
			case EmergeEDGE::Left:
				position.x = -ENEMY_WIDTH;
				position.y = rand() % WINDOW_HEIGHT;
				break;
			case EmergeEDGE::Right:
				position.x = WINDOW_WIDTH;
				position.y = rand() % WINDOW_HEIGHT;
				break;
			default:
				break;
			}
		}
		~Enemy()
		{
			delete anim_left;
			delete anim_right;
		}
		bool CheckBulletHit(const Bullet& bullet)
		{
			//子弹，判断子弹是否在敌人的矩形边界中
			bool is_cover_x = bullet.position.x >= position.x && bullet.position.x <= position.x + ENEMY_WIDTH;
			bool is_cover_y = bullet.position.y >= position.y && bullet.position.y <= position.y + ENEMY_HEIGHT;
			return is_cover_x && is_cover_y;
		}
		bool CheckPlayerHit(const Player& player)//受击碰撞箱
		{
			//取敌人中心为碰撞检测点位
			POINT check_position = { position.x + ENEMY_WIDTH / 2,position.y + ENEMY_HEIGHT / 2 };
			bool is_cover_x = check_position.x >= player.GetPosition().x && check_position.x <= player.GetPosition().x + player.PLAYER_WIDTH;
			bool is_cover_y = check_position.y >= player.GetPosition().y && check_position.y <= player.GetPosition().y+player.PLAYER_HEIGHT;
			return is_cover_x&& is_cover_y;
		}

		void Move(const Player& player)//敌人用来追踪玩家的函数
		{
			//仿照了人物的move
			const POINT& player_position = player.GetPosition();
			int dir_x = player_position.x - position.x;
			int dir_y = player_position.y - position.y;
			double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
			if (len_dir != 0)
			{
				double normalized_x = dir_x / len_dir;
				double normalized_y = dir_y / len_dir;
				position.x += (int)(SPEED * normalized_x);
				position.y += (int)(SPEED * normalized_y);
			}
			if (dir_x < 0) facing_left = true;
			else facing_left = false;
		}

		void Draw(int delta)
		{
			if (facing_left) anim_left->Play(position.x, position.y, delta);
			else anim_right->Play(position.x, position.y, delta);
		}

		void Hurt()//敌人受伤
		{
			alive = false;
		}

		bool CheckAlive()//是否存活
		{
			return alive;
		}
		
	private:
		const int SPEED = 2;
		const int ENEMY_WIDTH = 80;
		const int ENEMY_HEIGHT = 80;


		Animation* anim_left;
		Animation* anim_right;
		POINT position = { 0,0 };
		bool facing_left = false;
		bool alive = true;
};

//生成新敌人
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
	const int INTERVAL = 100;
	static int counter = 0;
	if ((++counter) % INTERVAL == 0) enemy_list.push_back(new Enemy());
}

//更新子弹
void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player)
{
	const double RADIUS_SPEED = 0.0045;//径向速度
	const double TANGENT_SPEED = 0.0055;//切向速度
	double radius_interval = 2 * 3.1415 / bullet_list.size();//弧度间隔
	POINT player_position = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIUS_SPEED);//径向移动
	for (size_t i = 0;i < bullet_list.size();i++)
	{
		double rad = GetTickCount() * TANGENT_SPEED + radius_interval * i;//弧度
		bullet_list[i].position.x = player_position.x + player.PLAYER_WIDTH / 2 + (int)(radius * cos(rad));//R*COSα
		bullet_list[i].position.y = player_position.y + player.PLAYER_HEIGHT / 2 + (int)(radius * sin(rad));
	}
}

//绘制得分
void DrawPlayerScore(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("玩家得分：%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

int main()
{
	initgraph(1280, 1280, 0);

	//音乐部分
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);//加载
	mciSendString(_T("open mus/hit.mp3 alias hit"), NULL, 0, NULL);

	mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//播放


	bool running = true;

	Player player;
	ExMessage msg;
	IMAGE img_background;
	std::vector<Enemy*> enemy_list;

	std::vector<Bullet> bullet_list(3);

	int score = 0;



	loadimage(&img_background, _T("image/background.png"));

	BeginBatchDraw();

	while (running)
	{
		DWORD start_time = GetTickCount();
		while (peekmessage(&msg))
		{
			player.ProcessEvent(msg);
		}

		player.Move();
		UpdateBullets(bullet_list, player);
		TryGenerateEnemy(enemy_list);
		for (Enemy* enemy : enemy_list)//遍历
			enemy->Move(player);

		//检测敌人的碰撞逻辑
			//和玩家碰撞------------------------------------------------------------------------------------死亡
		for (Enemy* enemy : enemy_list)
		{
			if (enemy->CheckPlayerHit(player))
			{
				static TCHAR text[64];
				_stprintf_s(text, _T("玩家得分：%d"), score);
				MessageBox(GetHWnd(), _T("Game Over"),_T("失败了"), MB_OK);
				running =false;
				break;
			}
		}
			//和子弹碰撞
		for (Enemy* enemy : enemy_list)
		{
			for (const Bullet& bullet : bullet_list)
			{
				if (enemy->CheckBulletHit(bullet))
				{
					mciSendString(_T("play hit from 0"), NULL, 0, NULL);//播放击打音效
					enemy->Hurt();
				}
			}
		}
		//清除生命值为零的敌人
		for (size_t i = 0;i < enemy_list.size();i++)
		{
			Enemy* enemy = enemy_list[i];
			if (!enemy->CheckAlive())
			{
				std::swap(enemy_list[i], enemy_list.back());
				enemy_list.pop_back();
				delete enemy;//切记delete
				score++;//得分这一块
			}
		}

		cleardevice();

		putimage(0, 0, &img_background);
		player.Draw(1000/144);
		for (Enemy* enemy : enemy_list)//遍历
			enemy->Draw(1000 / 144);
		for (const Bullet& bullet : bullet_list)
			bullet.Draw();
		DrawPlayerScore(score);

		FlushBatchDraw();


		DWORD end_time = GetTickCount();
		DWORD delta_time = end_time - start_time;
		if (delta_time < 1000 / 144)
		{
			Sleep(1000 / 144 - delta_time);
		}
	}

	EndBatchDraw();

	return 0;
}