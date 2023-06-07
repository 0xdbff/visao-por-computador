#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}


void vc_timer(void) {
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running) {
		running = true;
	}
	else {
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

		// Tempo em segundos.
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
		double nseconds = time_span.count();

		std::cout << "Tempo decorrido: " << nseconds << "segundos" << std::endl;
		std::cout << "Pressione qualquer tecla para continuar...\n";
		std::cin.get();
	}
}


int main(void) {
	// Vídeo
	char videofile[20] = "video.avi";
	cv::VideoCapture capture;
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;
	// Outros
	std::string str;
	int key = 0;

	/* Leitura de vídeo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video.avi deverá estar localizado no mesmo directório que o ficheiro de código fonte.
	*/
	capture.open(videofile);

	/* Em alternativa, abrir captura de vídeo pela Webcam #0 */
	//capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi possível abrir o ficheiro de vídeo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	/* Número total de frames no vídeo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do vídeo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolução do vídeo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o vídeo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	/* Inicia o timer */
	vc_timer();

	cv::Mat frame;
	while (key != 'q') {
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		/* Exemplo de inserção texto na frame */
		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);


		// Faça o seu código aqui...
		/*
		// Cria uma nova imagem IVC
		IVC *image = vc_image_new(video.width, video.height, 3, 255);
		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(image->data, frame.data, video.width * video.height * 3);
		// Executa uma função da nossa biblioteca vc
		vc_rgb_get_green(image);
		// Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
		memcpy(frame.data, image->data, video.width * video.height * 3);
		// Liberta a memória da imagem IVC que havia sido criada
		vc_image_free(image);
		*/
		// +++++++++++++++++++++++++

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

	/* Para o timer e exibe o tempo decorrido */
	vc_timer();

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de vídeo */
	capture.release();

	return 0;
}
