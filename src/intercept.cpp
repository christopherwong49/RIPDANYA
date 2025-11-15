#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

int main() {
	int pipe_to_child[2], pipe_from_child[2];
	pipe(pipe_to_child);
	pipe(pipe_from_child);

	pid_t pid = fork();
	if (pid == 0) { // Child process
		close(pipe_to_child[1]);
		close(pipe_from_child[0]);
		dup2(pipe_to_child[0], STDIN_FILENO);
		dup2(pipe_from_child[1], STDOUT_FILENO);
		close(pipe_to_child[0]);
		close(pipe_from_child[1]);
		execlp("/home/wdotmathree/uw/RIPDANYA/src/engine/ripdanya", "ripdanya", nullptr);
		perror("execlp failed");
		exit(1);
	} else { // Parent process
		close(pipe_to_child[0]);
		close(pipe_from_child[1]);

		// Make stdin and child stdout non-blocking
		fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
		fcntl(pipe_from_child[0], F_SETFL, fcntl(pipe_from_child[0], F_GETFL) | O_NONBLOCK);

		std::ofstream logfile("/home/wdotmathree/uw/RIPDANYA/logfile", std::ios::app);
		std::string accum_a, accum_b;

		auto a = [&]() {
			while (true) {
				char buffer[1024];
				ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
				if (n > 0) {
					std::string chunk(buffer, n);
					accum_a += chunk;
					size_t pos = 0;
					while ((pos = accum_a.find('\n')) != std::string::npos) {
						std::string line = accum_a.substr(0, pos);
						logfile << ">>> " << line << "\n" << std::flush;
						accum_a.erase(0, pos + 1);
					}
					write(pipe_to_child[1], buffer, n);
				} else if (n == 0) {
					break;
				} else {
					usleep(10000); // Sleep for 10ms
				}
			}
		};

		auto b = [&]() {
			while (true) {
				char buffer[1024];
				ssize_t n = read(pipe_from_child[0], buffer, sizeof(buffer));
				if (n > 0) {
					std::string chunk(buffer, n);
					accum_b += chunk;
					size_t pos = 0;
					while ((pos = accum_b.find('\n')) != std::string::npos) {
						std::string line = accum_b.substr(0, pos);
						logfile << "<<< " << line << "\n" << std::flush;
						accum_b.erase(0, pos + 1);
					}
					write(STDOUT_FILENO, buffer, n);
				} else if (n == 0) {
					break;
				} else {
					usleep(10000); // Sleep for 10ms
				}
			}
		};

		std::thread t1(a);
		std::thread t2(b);
		t1.join();
		t2.join();

		close(pipe_to_child[1]);
		close(pipe_from_child[0]);
		waitpid(pid, nullptr, 0);
		logfile.close();
	}
	return 0;
}
