#include <stdio.h> // standard input/output 
#include <stdlib.h> // dynamic allocation
#include <string.h> //string manipulation
#include <dirent.h> //directory(playlist) manipulation
#include <unistd.h> // file manipulation
#include <limits.h> // file manipulation contains PATH_MAX
#include <sys/stat.h> // symlink manipulation

#include "miniaudio.c" // for playing the songs

#define MAX_NAME 200
#define MAX_SONGS 10


//helper functions
char* path_name(const char* direct, char* songName, char* buffer, size_t length){
	/**
	getcwd(buffer, length);
	strcat(buffer, "/");
	strcat(buffer, songName);
	**/
	//snprintf(destination, length, format, variables....)
	snprintf(buffer, length, "%s/%s", direct, songName);
	return buffer;
}

typedef struct Song_Node {
	char song_abpath[PATH_MAX];
	char song_file[PATH_MAX];
	char song_name[MAX_NAME];
	struct Song_Node* song_prev;
	struct Song_Node* song_next;
} Song;

typedef struct Playlist_Node {
	char PL_abpath[PATH_MAX];
	char PL_file[PATH_MAX];
	char PL_name[MAX_NAME];
	Song* PL_songhead;
	Song* PL_currsong;
} Playlist;

typedef struct AudioControl_Node {
	ma_device AC_device;
	ma_decoder AC_decoder;
	Playlist* AC_playlist;
	int AC_isplaying;
} AudioControl;

//audio functions
void data_PLcallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
        AudioControl* audio_manager = (AudioControl*)pDevice->pUserData;
        if(audio_manager == NULL || audio_manager->AC_playlist->PL_currsong == NULL){
                return;
        }
        ma_uint64 framesRead;
        ma_result result = ma_decoder_read_pcm_frames(&audio_manager->AC_decoder, pOutput, frameCount, &framesRead);

        if(framesRead < frameCount){
                audio_manager->AC_playlist->PL_currsong = audio_manager->AC_playlist->PL_currsong->song_next;
                if(audio_manager->AC_playlist->PL_currsong != NULL){
                        ma_decoder_uninit(&audio_manager->AC_decoder);
                        ma_result initResult = ma_decoder_init_file(audio_manager->AC_playlist->PL_currsong->song_file, NULL, &audio_manager->AC_decoder);
                        if(initResult != MA_SUCCESS){
                                audio_manager->AC_isplaying = 0;
                        }
                }else{
                        audio_manager->AC_isplaying = 1;
                }
        }
}

//functions for Song struct
Song* create_song_node(const char* absolute_path, const char* file, const char* songName) {
	Song* new_song = (Song*)malloc(sizeof(Song));
	if (new_song == NULL) 
		return NULL;
	
	strncpy(new_song->song_abpath, absolute_path, PATH_MAX);
	strncpy(new_song->song_file, file, PATH_MAX);
	strncpy(new_song->song_name, songName, MAX_NAME -1);
	
	new_song->song_abpath[strlen(new_song->song_abpath) + 1] = '\0';
	new_song->song_file[strlen(new_song->song_file) + 1] = '\0';
	new_song->song_name[strlen(new_song->song_name) + 1] = '\0';
	
	new_song->song_prev = NULL;
	new_song->song_next = NULL;

	return new_song;
}

void insert_at_beginning(Song** head, const char* absolute_path, const char* file, const char* songName){
	Song* new_song = create_song_node(absolute_path, file, songName);
	if(*head == NULL){
		*head = new_song;
		return;
	}
	new_song->song_next = *head;
	(*head)->song_prev = new_song;
	new_song->song_prev = NULL;
	*head = new_song;
	return;
}

void insert_at_end(Song** head, const char* absolute_path, const char* file, const char* songName){
	Song* new_song = create_song_node(absolute_path, file, songName);
	if(*head == NULL){
		*head = new_song;
		return;
	}
	Song* temp = *head;
	while(temp->song_next != NULL){
		temp = temp->song_next;
	}
	temp->song_next = new_song;
	new_song->song_prev = temp;
}

void insert_at_position(Song** head, const char* absolute_path, const char* file, const char* songName, int position){
	if(position < 1){
		printf("Position ERROR\n");
		return;
	}else if(position == 1){
		insert_at_beginning(head, absolute_path, file, songName);
		return;
	}
	
	Song* new_song = create_song_node(absolute_path, file, songName);
	Song* temp = *head;
	for(int i = 1; temp != NULL && i < position - 1; i++){
		temp = temp->song_next;
	}
	if(temp == NULL){
		printf("Position ERROR\n");
		return;
	}

	new_song->song_next = temp->song_next;
	new_song->song_prev = temp;
	if(temp->song_next != NULL){
		temp->song_next->song_prev = new_song;
	}
	temp->song_next = new_song;
}

void delete_at_beginning(Song** head){
	if(*head == NULL){
		printf("EMPTY\n");
		return;
	}

	Song* temp = *head;
	*head = (*head)->song_next;
	if(*head != NULL){
		(*head)->song_prev = NULL;
	}
	free(temp);
}

void delete_at_end(Song** head){
	if(*head == NULL){
		printf("EMPTY\n");
		return;
	}
	
	Song* temp = *head;
	if(temp->song_next == NULL){
		*head = NULL;
		free(temp);
		return;
	}
	while(temp->song_next != NULL){
		temp = temp->song_next;
	}
	temp->song_prev->song_next = NULL;
	free(temp);
}

void delete_at_position(Song** head, int position){
	if(*head == NULL){
		printf("EMPTY\n");
		return;
	}
	Song* temp = *head;
	for(int i = 1; temp != NULL && i < position; i++){
		temp = temp->song_next;
	}
	if(temp == NULL){
		printf("Position ERROR\n");
		return;
	}
	if(temp->song_next != NULL){
		temp->song_next->song_prev = temp->song_prev;
	}
	if(temp->song_prev != NULL){
		temp->song_prev->song_next = temp->song_next;
	}
	free(temp);
}

void print_list_forward(Song* head){
	Song* temp = head;
	printf("Playlist contains songs: \n");
	while(temp != NULL){
		printf("\t%s\n", temp->song_name);
		temp = temp->song_next;
	}
	printf("\n");
}

void print_list_reverse(Song* head){
	Song* temp = head;
	if(temp == NULL){
		printf("EMPTY\n");
		return;
	}
	while(temp->song_next != NULL){
		temp = temp->song_next;
	}
	printf("Playlist constins songs: \n");
	while(temp != NULL){
		printf("\t%s\n", temp->song_name);
		temp = temp->song_next;
	}
	printf("\n");
}

//functions for Playlist struct
Playlist* create_playlist_node(const char* playlist_abpath, const char* playlist_file, const char* playlist_name, Song* playlist_songhead, Song* playlist_currentsong){
	Playlist* new_playlist = (Playlist*)malloc(sizeof(Playlist));
	if(new_playlist == NULL){
		return NULL;
	}

	strncpy(new_playlist->PL_abpath, playlist_abpath, PATH_MAX);
	strncpy(new_playlist->PL_file, playlist_file, PATH_MAX);
	strncpy(new_playlist->PL_name, playlist_name, MAX_NAME - 1);

	new_playlist->PL_abpath[strlen(new_playlist->PL_abpath) + 1] = '\0';
	new_playlist->PL_file[strlen(new_playlist->PL_file) + 1] = '\0';
	new_playlist->PL_name[strlen(new_playlist->PL_name) + 1] = '\0';

	new_playlist->PL_songhead = playlist_songhead;
	new_playlist->PL_currsong = playlist_currentsong;

	return new_playlist;
}

//all display functions
void print_playlist_data(Playlist* playlist){
	if (playlist == NULL){
		printf("Playlist NULL\n");
		return;
	}
	printf("Playlist absolute path: %s\n", playlist->PL_abpath);
	printf("Playlist relative path: %s\n", playlist->PL_file);
	printf("Playlist name: %s\n", playlist->PL_name);
	if(playlist->PL_songhead == NULL){
		printf("Playlist songhead is NULL\n");
	}else{
		printf("Playlist songhead : %s\n", playlist->PL_songhead->song_name);
	}
	if(playlist->PL_currsong == NULL){
		printf("Playlist currsong is NULL\n");
	}else{
		printf("Playlist currsong : %s\n", playlist->PL_currsong->song_name);
	}
}

void print_song_data(Song* song){
	if (song == NULL){
		printf("Song NULL\n");
		return;
	}
	printf("Song absolute path: %s\n", song->song_abpath);
	printf("Song relative path: %s\n", song->song_file);
	printf("Song name: %s\n", song->song_name);
	if(song->song_prev == NULL){
		printf("Song previous song is NULL\n");
	}else{
		printf("Song previous song : %s\n", song->song_prev->song_name);
	}
	if(song->song_next == NULL){
		printf("Song next song is NULL\n");
	}else{
		printf("Song next song : %s\n", song->song_next->song_name);
	}
}
