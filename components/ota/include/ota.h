#ifndef OTA
#define OTA

// void send_at(const char *cmd, int timeout);
// long get_length_file(const char* file_link, int timeout);
// int send_at_get_data(const char *cmd, int timeout, int chunk);
// int readAndSaveToFlash(int file_size);
void OTA_from_link_v2(char *file_link, int timeout);
#endif