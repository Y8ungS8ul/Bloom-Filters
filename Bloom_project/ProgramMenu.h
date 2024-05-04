#pragma once
#include <SFML/Graphics.hpp>

namespace MainMenu 
{
	class AppMenu
	{
		float menu_X; //координаты меню по Ох
		float menu_Y; //координаты меню по Оу
		int menu_Step; //расстояние между пунктами меню
		int max_menu; // максимальное количество пунктов меню
		int size_font; //размер шрифта
		int mainMenuSelected; //номер текущего пункта меню
		sf::Font font; //шрифт меню
		sf::Text* mainMenu; //динамический массив текстовых объектов нахваний пунктов меню
		sf::Color menu_text_color = sf::Color::White; //цвет пунктов меню
		sf::Color chose_text_color = sf::Color::Yellow; //цвет выбора пункта меню
		sf::Color border_color = sf::Color::Black; //цвет обводки текста пунктов меню

		//настройка текста пунктов меню
		//Параметры: ссылка на текстовый объект, текст, координаты текста

		void setInitText(sf::Text& text, sf::String str, float xpos, float ypos);
		// Ссылка на графическое окно
		sf::RenderWindow& mywindow;
	public:
		// Конструктор 
		// Параметры: ссылка на графическое окно, координаты игрового меню по x и y
		// количество пунктов меню, массив названий пунктов меню, размер шрифта, шаг между пунктами меню
		AppMenu(sf::RenderWindow& window, float menux, float menuy,
			int index, sf::String name[], int sizeFont = 60, int step = 50);

		~AppMenu()
		{
			delete[] mainMenu;
		}

		void draw();        //Метод отрисовки меню

		void MoveUp();     //Перемещение выбора меню вверх

		void MoveDown();   //Перемещение выбора меню вниз

		// Цвет элементов игрового меню
		void setColorTextMenu(sf::Color menColor, sf::Color ChoColor,
			sf::Color BordColor);

		void AlignMenu(int posx, float width);   // Выравнивание положения меню

		int getSelectedMenuNumber() // Возвращает номер выбранного элемента меню
		{
			return mainMenuSelected;
		}
	};
}