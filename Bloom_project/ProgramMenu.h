#pragma once
#include <SFML/Graphics.hpp>

namespace MainMenu 
{
	class AppMenu
	{
		float menu_X; //���������� ���� �� ��
		float menu_Y; //���������� ���� �� ��
		int menu_Step; //���������� ����� �������� ����
		int max_menu; // ������������ ���������� ������� ����
		int size_font; //������ ������
		int mainMenuSelected; //����� �������� ������ ����
		sf::Font font; //����� ����
		sf::Text* mainMenu; //������������ ������ ��������� �������� �������� ������� ����
		sf::Color menu_text_color = sf::Color::White; //���� ������� ����
		sf::Color chose_text_color = sf::Color::Yellow; //���� ������ ������ ����
		sf::Color border_color = sf::Color::Black; //���� ������� ������ ������� ����

		//��������� ������ ������� ����
		//���������: ������ �� ��������� ������, �����, ���������� ������

		void setInitText(sf::Text& text, sf::String str, float xpos, float ypos);
		// ������ �� ����������� ����
		sf::RenderWindow& mywindow;
	public:
		// ����������� 
		// ���������: ������ �� ����������� ����, ���������� �������� ���� �� x � y
		// ���������� ������� ����, ������ �������� ������� ����, ������ ������, ��� ����� �������� ����
		AppMenu(sf::RenderWindow& window, float menux, float menuy,
			int index, sf::String name[], int sizeFont = 60, int step = 50);

		~AppMenu()
		{
			delete[] mainMenu;
		}

		void draw();        //����� ��������� ����

		void MoveUp();     //����������� ������ ���� �����

		void MoveDown();   //����������� ������ ���� ����

		// ���� ��������� �������� ����
		void setColorTextMenu(sf::Color menColor, sf::Color ChoColor,
			sf::Color BordColor);

		void AlignMenu(int posx, float width);   // ������������ ��������� ����

		int getSelectedMenuNumber() // ���������� ����� ���������� �������� ����
		{
			return mainMenuSelected;
		}
	};
}