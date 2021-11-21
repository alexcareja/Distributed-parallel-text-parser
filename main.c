#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define MASTER 0
#define NUM_THREADS 4
#define NUM_WORKERS 4
#define HORROR "horror\n"
#define COMEDY "comedy\n"
#define FANTASY "fantasy\n"
#define SF "science-fiction\n"
#define NEWLINE "\n"
#define MAX_LINELEN 2500
#define PARAGRAPH 0
#define LINELEN 1
#define TEXT 2
#define LINES_ASSIGNED 20
#define TYPE_HORROR 0
#define TYPE_COMEDY 1
#define TYPE_FANTASY 2
#define TYPE_SF 3

static char in_file[100];
static char out_file[100];
static char ***par;
static int *par_lines_size, *par_order, *par_lines_cap;
static int par_cap, par_size;
static pthread_mutex_t lock;
static pthread_barrier_t barrier;
static int shared_counter = 0, shared_par = 0;

int is_letter(char);
int is_vowel(char);
int is_consonant(char);
char get_lowercase(char);
char get_uppercase(char);
void *master_function(void *);
void receive_data();
void send_data();
void process_horror(int, int, int);
void process_comedy(int, int, int);
void process_fantasy(int, int, int);
void process_sf(int, int, int);
void parallel_process(int, int);
void *horror(void *);
void *comedy(void *);
void *fantasy(void *);
void *science_fiction(void *);

int main (int argc, char *argv[])
{
	int procs, rank, provided, r, i, j;

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_size(MPI_COMM_WORLD, &procs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	r = pthread_mutex_init(&lock, NULL);
	if (r) {
		exit(-1);
	}

	if (rank == MASTER) {
		strcpy(in_file, argv[1]);
		int thread_id[NUM_THREADS];
		pthread_t tid[NUM_THREADS];

		// Init barrier
		r = pthread_barrier_init(&barrier, NULL, NUM_THREADS);
		if (r) {
			exit(-1);
		}

		// Start 4 threads in master
		for (i = 0; i < NUM_THREADS; ++i) {
			thread_id[i] = i;
			r = pthread_create(&tid[i], NULL, master_function, (void *) &thread_id[i]);
			if (r) {
				exit(-1);
			}
		}

		for (i = 0; i < NUM_THREADS; ++i) {
			r = pthread_join(tid[i], NULL);
			if (r) {
				exit(-1);
			}
		}

		// Write processed text to out file
		strcpy(out_file, in_file);
		int x = strlen(out_file);
		// Change out file
		out_file[x - 1] = 't';
		out_file[x - 2] = 'u';
		out_file[x - 3] = 'o';
		FILE *f = fopen(out_file, "w");
		for (i = 0; i < par_size; ++i) {
			for (j = 0; j < par_lines_size[i]; ++j) {
				fprintf(f, "%s", par[i][j]);
			}
			if (i == par_size - 1) {
				break;
			}
			fprintf(f, "\n");
		}
		fclose(f);
	} else {
		int max_threads = (int) sysconf(_SC_NPROCESSORS_CONF);
		int thread_id[max_threads];
		pthread_t tid[max_threads];

		// Alloc vector of paragraphs
		par = malloc(sizeof(char **));
		// Alloc vector of paragraph orders
		par_order = calloc(1, sizeof(int));
		// Alloc vector of paragraphs sizes
		par_lines_size = calloc(1, sizeof(int));
		// Alloc vector of paragraphs capacities
		par_lines_cap = calloc(1, sizeof(int));
		par_cap = 1;

		// Init barrier
		r = pthread_barrier_init(&barrier, NULL, max_threads);
		if (r) {
			exit(-1);
		}

		// Create `max_threads` threads
		for (i = 0; i < max_threads; ++i) {
			thread_id[i] = i;
			switch (rank) {
			case 1:
				r = pthread_create(&tid[i], NULL, horror, (void *) &thread_id[i]);
				break;
			case 2:
				r = pthread_create(&tid[i], NULL, comedy, (void *) &thread_id[i]);
				break;
			case 3:
				r = pthread_create(&tid[i], NULL, fantasy, (void *) &thread_id[i]);
				break;
			case 4:
				r = pthread_create(&tid[i], NULL, science_fiction, (void *) &thread_id[i]);
				break;
			default:
				r = 1;
				break;
			}
			if (r) {
				exit(-1);
			}
		}

		for (i = 0; i < max_threads; ++i) {
			r = pthread_join(tid[i], NULL);
			if (r) {
				exit(-1);
			}
		}
		// Free allocated memory
		free(par_order);
		free(par_lines_cap);
		for (i = 0; i < par_size; ++i) {
			for (j = 0; j < par_lines_size[i]; ++j) {
				free(par[i][j]);
			}
			free(par[i]);
		}
		free(par);
		free(par_lines_size);
	}

	// Destroy mutex
	pthread_mutex_destroy(&lock);
	// Destroy barrier
	pthread_barrier_destroy(&barrier);

	MPI_Finalize();

	return 0;
}

int is_letter(char c) {
	// Return 1 if `c` is letter, 0 otherwise
	return ((c > 64 && c < 91) || (c > 96 && c < 123)) ? 1 : 0;
}

int is_vowel(char c) {
	// Return 1 if `c` is vowell, 0 otherwise
	if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
	        c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U') {
		return 1;
	}
	return 0;
}

int is_consonant(char c) {
	// Return 1 if consonant, 0 otherwise
	return (is_letter(c) && is_vowel(c) == 0) ? 1 : 0;
}

char get_lowercase(char c) {
	// Return lowercase(c)
	return (c > 96) ? c : c + 32;
}

char get_uppercase(char c) {
	// Return uppercase(c)
	return (c < 91) ? c : c - 32;
}

void *master_function(void *arg) {
	int id = *(int *) arg, linelen, i, j;
	char *genre;
	char *line;
	char *buffer = calloc(MAX_LINELEN, 1);
	size_t r, length = 0;
	int my_genre = 0;		// 1 if paragraph is dedicated to this thread
	int par_counter = 0;	// Counter for number of paragraphs found
	int msg_cnt = 0;		// Tag for every MPI message / messages counter

	// Get genre based on id (0 = horror, 1 = comedy etc.)
	switch (id) {
		case 0:
			genre = calloc(strlen(HORROR) + 1, 1);
			strcpy(genre, HORROR);
			break;
		case 1:
			genre = calloc(strlen(COMEDY) + 1, 1);
			strcpy(genre, COMEDY);
			break;
		case 2:
			genre = calloc(strlen(FANTASY) + 1, 1);
			strcpy(genre, FANTASY);
			break;
		case 3:
			genre = calloc(strlen(SF) + 1, 1);
			strcpy(genre, SF);
			break;
		default:
			break;
	}

	// Open input file and start reading
	FILE *f = fopen(in_file, "r");
	if (f != NULL) {
		while ((r = getline(&line, &length, f)) != -1) {
			// Copy line in buffer
			strcpy(buffer, line);
			if (strcmp(buffer, NEWLINE) == 0) {
				// If newline => new paragraph
				par_counter++;
			}
			if (strcmp(buffer, genre) == 0) {
				// If buffer == genre => This next paragraph is dedicated to this thread
				my_genre = 1;
				// Send paragraph index order
				MPI_Send(&par_counter, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD);
				msg_cnt++;
				continue;
			}
			if (my_genre && strcmp(buffer, NEWLINE) == 0) {
				// If the paragraph was dedicated to this thread and it encounters NEWLINE line 
				// Then => end of paragraph
				my_genre = 0;
				int x = -1;	// End of paragraph
				MPI_Send(&x, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD);
				msg_cnt++;
			}
			if (my_genre) {
				// If this paragraph is dedicated to me, then send lines to worker
				linelen = strlen(buffer) + 1;
				// Send length of line
				MPI_Send(&linelen, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD);
				msg_cnt++;
				// Send line
				MPI_Send(buffer, r + 1, MPI_CHAR, id + 1, msg_cnt, MPI_COMM_WORLD);
				msg_cnt++;
				// Set buffer to ""
				memset(buffer, 0, MAX_LINELEN);
			}
		}
		if (my_genre) {
			j = -1;	// End of last paragraph
			MPI_Send(&j, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD);
			msg_cnt++;
		}
	}
	par_counter++;
	j = -1;	// End of transmission
	MPI_Send(&j, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD);
	fclose(f);

	if (line) {
		free(line);
	}

	// Set global variable to number of paragraphs found
	pthread_mutex_lock(&lock);
	par_size = par_counter;
	pthread_mutex_unlock(&lock);

	int pars_no, par_ord, lines;
	msg_cnt = 0;
	if (id == 0) {
		// Thread 0 allocs space for the paragraphs
		par = malloc(par_counter * sizeof(char **));
		par_lines_size = calloc(par_counter, sizeof(int));
	}
	// All threads wait
	pthread_barrier_wait(&barrier);
	// Rceive number of paragraphs
	MPI_Recv(&pars_no, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD, NULL);
	msg_cnt++;
	for (i = 0; i < pars_no; ++i) {
		// Receive paragraph index order
		MPI_Recv(&par_ord, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD, NULL);
		msg_cnt++;
		// Receive number of lines in current paragraph
		MPI_Recv(&lines, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD, NULL);
		msg_cnt++;
		lines++;
		par_lines_size[par_ord] = lines;
		// Alloc memory for the lines strings
		par[par_ord] = malloc(lines * sizeof(char *));
		// First line of each paragraph is the genre
		par[par_ord][0] = calloc(strlen(genre) + 1, 1);
		strcpy(par[par_ord][0], genre);
		for (j = 1; j < lines; ++j) {
			// Receive length of line
			MPI_Recv(&linelen, 1, MPI_INT, id + 1, msg_cnt, MPI_COMM_WORLD, NULL);
			msg_cnt++;
			par[par_ord][j] = calloc(linelen + 1, 1);
			// Receive line string
			MPI_Recv(buffer, linelen, MPI_CHAR, id + 1, msg_cnt, MPI_COMM_WORLD, NULL);
			msg_cnt++;
			strcpy(par[par_ord][j], buffer);
			// Set buffer to ""
			memset(buffer, 0, MAX_LINELEN);
		}
	}

	// Free allocated memory
	free(buffer);
	free(genre);

	pthread_exit(NULL);
}

void receive_data() {
	int linelen, paragraph_no, msg_cnt = 0;
	char *received = calloc(MAX_LINELEN, 1);
	while (1) {
		// Receive paragraph index order
		MPI_Recv(&paragraph_no, 1, MPI_INT, MASTER, msg_cnt, MPI_COMM_WORLD, NULL);
		msg_cnt++;
		if (paragraph_no == -1) {
			// If end of communication, then break
			break;
		}
		par_order[par_size] = paragraph_no;
		if (par_size == par_cap) {
			// If paragraph vector is full, then double its size + realloc
			par_cap *= 2;

			// Realloc array of paragraphs
			par = realloc(par, par_cap * sizeof(char **));

			// Realloc array of ints which represent number of lines in each paragraph
			par_lines_size = realloc(par_lines_size, par_cap * sizeof(int));

			// Realloc array of ints which represent capacity of lines in each paragraph
			par_lines_cap = realloc(par_lines_cap, par_cap * sizeof(int));

			// Realloc array of ints which represent indexes of paragraphs
			par_order = realloc(par_order, par_cap * sizeof(int));
		}
		// Alloc memory for this paragraph
		par[par_size] = malloc(sizeof(char *));
		par_lines_size[par_size] = 0;
		par_lines_cap[par_size] = 1;
		while (1) {
			// Receive length of line
			MPI_Recv(&linelen, 1, MPI_INT, MASTER, msg_cnt, MPI_COMM_WORLD, NULL);
			msg_cnt++;
			if (linelen == -1) {
				// If received -1 => end of paragraph
				break;
			}
			if (par_lines_size[par_size] == par_lines_cap[par_size]) {
				// If lines vector is full, then double its size
				par_lines_cap[par_size] *= 2;
				// Realloc array of lines
				par[par_size] = realloc(par[par_size], par_lines_cap[par_size] * sizeof(char *));
			}
			// Alloc space for next line
			par[par_size][par_lines_size[par_size]] = calloc(2 * linelen + 2, 1);
			// Receive next line
			MPI_Recv(received, linelen + 1, MPI_CHAR, MASTER, msg_cnt, MPI_COMM_WORLD, NULL);
			msg_cnt++;
			strcpy(par[par_size][par_lines_size[par_size]], received);
			par_lines_size[par_size]++;
			// Set buffer to ""
			memset(received, 0, MAX_LINELEN);
		}
		par_size++;
	}
	// Finished receiving data
	free(received);
}

void send_data() {
	int i, j, msg_cnt = 0, len;
	MPI_Send(&par_size, 1, MPI_INT, MASTER, msg_cnt, MPI_COMM_WORLD);
	msg_cnt++;
	for (i = 0; i < par_size; ++i) {
		// Send paragraph index
		MPI_Send(&par_order[i], 1, MPI_INT, MASTER, msg_cnt, MPI_COMM_WORLD);
		msg_cnt++;
		// Send number of lines in paragraph
		MPI_Send(&par_lines_size[i], 1, MPI_INT, MASTER, msg_cnt, MPI_COMM_WORLD);
		msg_cnt++;
		for (j = 0; j < par_lines_size[i]; ++j) {
			len = strlen(par[i][j]) + 1;
			// Send length on current line
			MPI_Send(&len, 1, MPI_INT, MASTER, msg_cnt, MPI_COMM_WORLD);
			msg_cnt++;
			// Send current line
			MPI_Send(par[i][j], len, MPI_CHAR, MASTER, msg_cnt, MPI_COMM_WORLD);
			msg_cnt++;
		}
	}
}

void process_horror(int start, int end, int current_par) {
	// Process horror text from start line to end line in the `current_par` paragraph
	int i, j, k;
	char *buffer = calloc(MAX_LINELEN, 1);
	for (i = start; i < end; ++i) {
		// For every line
		memset(buffer, 0, MAX_LINELEN);
		strcpy(buffer, par[current_par][i]);
		j = -1;
		k = 0;
		do {
			j++;
			// Copy next char
			par[current_par][i][k++] = buffer[j];
			if (is_consonant(buffer[j])) {
				// Duplicate the consonant with its lowercase char
				par[current_par][i][k++] = get_lowercase(buffer[j]);
			}
		} while (buffer[j] != '\0');
	}
	free(buffer);
}

void process_comedy(int start, int end, int current_par) {
	// Process comedy text from start line to end line in the `current_par` paragraph
	int i, j, k, parity;
	char *buffer = calloc(MAX_LINELEN, 1);
	for (i = start; i < end; ++i) {
		memset(buffer, 0, MAX_LINELEN);
		strcpy(buffer, par[current_par][i]);
		j = -1;
		k = 0;
		parity = 0;
		do {
			j++;
			if (buffer[j] == ' ' || buffer[j] == '\t' || buffer[j] == '\n') {
				// If new word, then reset parity
				parity = 0;
			} else {
				// Else, increment parity
				parity++;
			}
			if (parity % 2 == 0 && is_letter(buffer[j])) {
				// If its and even letter in the word then make it uppercase
				par[current_par][i][k++] = get_uppercase(buffer[j]);
			} else {
				// Else, copy it as is
				par[current_par][i][k++] = buffer[j];
			}
		} while (buffer[j] != '\0');
	}
	free(buffer);
}

void process_fantasy(int start, int end, int current_par) {
	// Process fantasy text from start line to end line in the `current_par` paragraph
	int i, j, k, new_word = 1;
	char *buffer = calloc(MAX_LINELEN, 1);
	for (i = start; i < end; ++i) {
		memset(buffer, 0, MAX_LINELEN);
		strcpy(buffer, par[current_par][i]);
		j = -1;
		k = 0;
		do {
			j++;
			if (new_word && is_letter(buffer[j])) {
				// If encountered a new word, then make its first letter uppercase
				par[current_par][i][k++] = get_uppercase(buffer[j]);
				new_word = 0;
			} else {
				// Else, copy the character as is
				par[current_par][i][k++] = buffer[j];
			}
			if (buffer[j] == ' ' || buffer[j] == '\t' || buffer[j] == '\n') {
				// If whitespace, then next char is start of new word
				new_word = 1;
			}
		} while (buffer[j] != '\0');
	}
	free(buffer);
}

void process_sf(int start, int end, int current_par) {
	// Process s-f text from start line to end line in the `current_par` paragraph
	int i, j, k, word_count, wstart, wend;
	char *buffer = calloc(MAX_LINELEN, 1);
	for (i = start; i < end; ++i) {
		memset(buffer, 0, MAX_LINELEN);
		strcpy(buffer, par[current_par][i]);
		j = -1;
		k = 0;
		word_count = 1;
		wstart = -1;
		wend = -1;
		do {
			j++;
			if (word_count == 7) {
				// If this is the 7th word then remember start of word and search for end of word
				if (wstart == -1) {
					wstart = j;
				}
				if (buffer[j] == ' ' || buffer[j] == '\t' || buffer[j] == '\n') {
					// If found end of word
					wend = j - 1;
					if (wend >= wstart) {
						// If it has at least one character
						for (; wend >= wstart; --wend) {
							// Copy the word reversed
							par[current_par][i][k++] = buffer[wend];
						}
					}
					// Copy the whitespace
					par[current_par][i][k++] = buffer[j];
					// Reset word count, and word start/end
					wstart = -1;
					wend = -1;
					word_count = 0;
				}
			} else {
				// If it's not the 7th word, then copy the text as is
				par[current_par][i][k++] = buffer[j];
			}
			if (buffer[j] == ' ' || buffer[j] == '\t' || buffer[j] == '\n') {
				// If whitespace => increment word count
				word_count++;
			}

		} while (buffer[j] != '\0');
	}
	free(buffer);
}

void parallel_process(int id, int genre) {
	// Handle parallel processing for every genre
	if (id == 0) {
		// Thread 0 receives data
		receive_data();
		pthread_barrier_wait(&barrier);
	} else {
		// The other P - 1 threads wait for the data to be received
		pthread_barrier_wait(&barrier);
		// Start processing the text
		int current_par = 0, start, end;
		while (current_par < par_size) {
			// Get hold of lock
			pthread_mutex_lock(&lock); {
				if (current_par != shared_par) {
					// Update current paragraph
					current_par = shared_par;
				}
				if (LINES_ASSIGNED * shared_counter >= par_lines_size[shared_par] ) {
					// If end of paragraph => next paragraph
					shared_par++;
					current_par = shared_par;
					shared_counter = 0;
				}
				if (LINES_ASSIGNED * shared_counter < par_lines_size[current_par] &&
					current_par < par_size) {
					// Self assign next 20 available lines from this paragraph to this thread
					start = LINES_ASSIGNED * shared_counter;
					end = LINES_ASSIGNED * shared_counter + LINES_ASSIGNED;
					if (end > par_lines_size[current_par]) {
						end = par_lines_size[current_par];
					}
					// Increment the shared counter
					shared_counter++;
				}
			}
			// Release lock
			pthread_mutex_unlock(&lock);
			// Process start -> end lines inside the `current_par` paragraph
			switch (genre) {
				case TYPE_HORROR:
					process_horror(start, end, current_par);
					break;
				case TYPE_COMEDY:
					process_comedy(start, end, current_par);
					break;
				case TYPE_FANTASY:
					process_fantasy(start, end, current_par);
					break;
				case TYPE_SF:
					process_sf(start, end, current_par);
					break;
				default:
					break;
			}
			start = 0;
			end = 0;
		}
	}
	// Wait for all threads to finish processing
	pthread_barrier_wait(&barrier);
	if (id == 0) {
		// Thread 0 will send the data back to MASTER
		send_data();
	}
}

void *horror(void *arg) {
	// Horror worker function
	int id = *(int *) arg;
	parallel_process(id, TYPE_HORROR);
	pthread_exit(NULL);
}

void *comedy(void *arg) {
	// Comedy worker function
	int id = *(int *) arg;
	parallel_process(id, TYPE_COMEDY);
	pthread_exit(NULL);
}

void *fantasy(void *arg) {
	// Fantasy worker function
	int id = *(int *) arg;
	parallel_process(id, TYPE_FANTASY);
	pthread_exit(NULL);
}

void *science_fiction(void *arg) {
	// Science-fiction worker function
	int id = *(int *) arg;
	parallel_process(id, TYPE_SF);
	pthread_exit(NULL);
}