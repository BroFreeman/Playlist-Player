#include "playlistplayer.c"
int main(int argc, char* argv[]){
	if(argc != 2){
		printf("ERROR ARGS!\n");
		return -1;
	}
	
	Playlist* playlist = NULL;
	Song* songHead = NULL;
	char buffer[PATH_MAX];
	char file[PATH_MAX];
	char cwd[PATH_MAX];
	char* name;
	
	DIR *dr = opendir(argv[1]);
	if(dr == NULL){
		printf("ERROR opening directory\n");
		return -2;
	}
	
	strncpy(file, argv[1], sizeof(file) - 1);
    file[sizeof(file) -1 ] = '\0';
    char* fileLastSlash = strrchr(file, '/');
    name = fileLastSlash ? fileLastSlash + 1 : file;
    getcwd(cwd, PATH_MAX);
    snprintf(buffer, PATH_MAX, "%s/%s", cwd, file);
    playlist = create_playlist_node(buffer, file, name, songHead, songHead);

	struct dirent *de;
	int songcount = 0;
	while((de = readdir(dr)) != NULL){
		char* point = strrchr(de->d_name, '.');
		if(point && strcmp(point + 1, "mp3") == 0){
			songcount++;
			path_name(playlist->PL_file, de->d_name, buffer, PATH_MAX);
			path_name(playlist->PL_file, de->d_name, file, PATH_MAX);
			if(songcount == 1){
				songHead = create_song_node(buffer, buffer, de->d_name);
			}else{
				insert_at_beginning(&songHead, buffer, buffer, de->d_name);
			}
		}
	}
	closedir(dr);
	
	if(songHead == NULL){
		printf("ERROR no readable song files\n");
		return -3;
	}

	playlist->PL_songhead = songHead;
	playlist->PL_currsong = songHead;

	printf("Songs list\n");
	print_list_forward(songHead);

	AudioControl audio_manager;
	audio_manager.AC_playlist = playlist;
	audio_manager.AC_isplaying = 1;
	
	ma_result result = ma_decoder_init_file(playlist->PL_currsong->song_file, NULL, &audio_manager.AC_decoder);
	if(result != MA_SUCCESS){
		printf("ERROR could not load files\n");
		return -4;
	}

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = audio_manager.AC_decoder.outputFormat;
	config.playback.channels = audio_manager.AC_decoder.outputChannels;
	config.sampleRate = audio_manager.AC_decoder.outputSampleRate;
	config.dataCallback = data_PLcallback;
	config.pUserData = &audio_manager;

	if(ma_device_init(NULL, &config, &audio_manager.AC_device) != MA_SUCCESS){
		printf("Failed to initialize device.\n");
		ma_decoder_uninit(&audio_manager.AC_decoder);
		return -5;
	}

	ma_device_start(&audio_manager.AC_device);

	printf("Playling playlist %s", playlist->PL_name);
	printf("Press enter to quit\n");
	getchar();

	audio_manager.AC_isplaying = 0;
	
	ma_device_uninit(&audio_manager.AC_device);
	ma_decoder_uninit(&audio_manager.AC_decoder);

	Song* curr = songHead;
	while(curr != NULL){
		Song* next = curr->song_next;
		free(curr);
		curr = next;
	}
	free(playlist);
	
	printf("Playlist end.\n");
	return 0;
}
