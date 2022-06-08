#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

void copyFile(char *toCopy, char *fName) // copy toCopy to fName
{
	FILE *fptr1, *fptr2;
	unsigned char buffer[255];
	size_t size;

	// Open one file for reading
	fptr1 = fopen(toCopy, "r");
	if (fptr1 == NULL)
	{
		printf("Cannot open file %s \n", toCopy);
		exit(0);
	}

	// Open another file for writing
	fptr2 = fopen(fName, "w");
	if (fptr2 == NULL)
	{
		printf("Cannot open file %s \n", fName);
		exit(0);
	}

	// Read contents from file
	while ((size = fread(buffer, 1, sizeof(buffer), fptr1)) > 0)
	{
		fwrite(buffer, 1, size, fptr2);
	}

	fclose(fptr1);
	fclose(fptr2);
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}

	char outP[sizeof(argv[2]) + 3];
	strcpy(outP, argv[2]);
	outP[sizeof(argv[2]) + 2] = '\0';

	char outP2[sizeof(argv[2]) + 3];
	strcpy(outP2, argv[2]);
	outP2[sizeof(argv[2]) + 2] = '\0';

	char outP3[sizeof(argv[2]) + 3];
	strcpy(outP3, argv[2]);
	outP3[sizeof(argv[2]) + 2] = '\0';

	int fd;

	fd = open(argv[1], O_RDONLY); /* open disk image */

	ext2_read_init(fd);

	// interate through each block group

	int jpegs[150]; // stores the inodes of the desired .jpg files
	int nextSpot = 0;

	int dirs[150]; // stores the inodes of all directory entries
	int dirNextSpot = 0;

	// make sure the given directory does not exist, and create it
	if (opendir(argv[2]) == NULL)
	{
		mkdir(argv[2], 0666);
	}
	else
	{
		printf("The given directory already exists.\n");
		exit(0);
	}

	for (unsigned int z = 0; z < num_groups; z++)
	{

		struct ext2_super_block super;
		struct ext2_group_desc group;

		// example read first the super-block and group-descriptor
		read_super_block(fd, z, &super);
		read_group_desc(fd, z, &group);

		off_t start_inode_table = locate_inode_table(z, &group);

		// iterate over all the files in the inode table
		for (unsigned int j = 0; j < inodes_per_group; j++) // inodes per group
		{
			char buffer[1024]; // stores the current inode's data

			struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
			read_inode(fd, z, start_inode_table, j, inode); // acquire the next inode from the table
			int s = inode->i_size;

			// unsigned int i_blocks = inode->i_blocks / (2 << super.s_log_block_size);
			// printf("number of blocks %u\n", i_blocks);
			// printf("Is directory? %s \n Is Regular file? %s\n",
			// 	   S_ISDIR(inode->i_mode) ? "true" : "false",
			// 	   S_ISREG(inode->i_mode) ? "true" : "false");

			if (S_ISDIR(inode->i_mode))
			{ // record the directory's inode and continue

				dirs[dirNextSpot++] = j;

				continue;
			}

			if (!S_ISREG(inode->i_mode))
			{ // the file is not a regular file, so move on to the next inode
				continue;
			}

			// the file is a regular file!

			lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET); // set the fd pointer to the next data block
			read(fd, buffer, 1024);

			if (buffer[0] == (char)0xff &&
				buffer[1] == (char)0xd8 &&
				buffer[2] == (char)0xff &&
				(buffer[3] == (char)0xe0 ||
				 buffer[3] == (char)0xe1 ||
				 buffer[3] == (char)0xe8))
			{ // the file is a jpg!

				jpegs[nextSpot++] = j;

				int num = j;
				char inode_file_name[14];

				char dir[50];

				sprintf(dir, "%s/", outP);

				snprintf(inode_file_name, 14, "file-%d", num);

				char *jpg = ".jpg";
				strcat(inode_file_name, jpg);

				strcat(dir, inode_file_name);

				int fp;
				fp = open(dir, O_WRONLY | O_APPEND | O_CREAT, 0666); // fp now points to the output file

				for (unsigned int i = 0; i < EXT2_N_BLOCKS; i++)
				{

					if (i < EXT2_NDIR_BLOCKS)
					{ /* direct blocks */

						// skip loop because does not point to valid
						if (inode->i_block[i] == 0)
						{
							continue;
						}

						if (s < 1024 && s > 0)
						{
							lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET); // set the fd pointer to the next data block //incorrect offset
							read(fd, buffer, s);

							write(fp, buffer, s);
							s -= 1024;
						}
						else if (s > 0)
						{
							lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET); // set the fd pointer to the next data block //incorrect offset
							read(fd, buffer, 1024);

							write(fp, buffer, 1024);
							s -= 1024;
						}
					}
					else if (i == EXT2_IND_BLOCK)
					{ /* single indirect block */

						lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET); // set the fd pointer to the next data block
						read(fd, buffer, 1024);

						char buffer1[1024];

						// need to go through until 0 is found

						int k = 0;

						while (k < 256 && ((int *)buffer)[k] != 0) // check k > 256
						{

							if (s < 1024 && s > 0)
							{
								lseek(fd, BLOCK_OFFSET(((int *)buffer)[k]), SEEK_SET); // set the fd pointer to the next data block
								read(fd, buffer1, s);								   // read and write up 1024 unless last block could be less

								write(fp, buffer1, s);
								s -= 1024;
							}
							else if (s > 0)
							{
								lseek(fd, BLOCK_OFFSET(((int *)buffer)[k]), SEEK_SET); // set the fd pointer to the next data block
								read(fd, buffer1, 1024);							   // read and write up 1024 unless last block could be less

								write(fp, buffer1, 1024);
								s -= 1024;
							}

							k++;
						}
					}
					else if (i == EXT2_DIND_BLOCK)
					{ /* double indirect block */

						lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET); // set the fd pointer to the next data block

						if (s < 1024 && s > 0)
						{
							read(fd, buffer, s);
						}
						else if (s > 0)
						{
							read(fd, buffer, 1024);
						}

						// need to go through until 0 is found

						int k = 0;

						// iterate through the buffer until empty block num is found
						while (k < 256)
						{
							if (((int *)buffer)[k] == 0)
							{
								k++;
								continue;
							}

							char buffer2[1024];
							lseek(fd, BLOCK_OFFSET(((int *)buffer)[k]), SEEK_SET); // set the fd pointer to the next data block
							
							if (s < 1024 && s > 0)
							{
								read(fd, buffer2, s);
							}
							else if (s > 0)
							{
								read(fd, buffer2, 1024);
							}

							int h = 0;

							while (h < 256 && ((int *)buffer2)[h] != 0)
							{

								if (s < 1024 && s > 0)
								{
									char buffer3[s];
									lseek(fd, BLOCK_OFFSET(((int *)buffer2)[h]), SEEK_SET); // set the fd pointer to the next data block
									read(fd, buffer3, s);

									write(fp, buffer3, s);
									s -= 1024;
								}
								else if (s > 0)
								{
									char buffer3[1024];
									lseek(fd, BLOCK_OFFSET(((int *)buffer2)[h]), SEEK_SET); // set the fd pointer to the next data block
									read(fd, buffer3, 1024);

									write(fp, buffer3, 1024);
									s -= 1024;
								}
								h++;
							}
							k++;
						}
					}
				}
			}
			free(inode);
		}
	} // now all the .jpg inodes are located

	struct ext2_inode *jpgInode = malloc(sizeof(struct ext2_inode));
	struct ext2_inode *dirInode = malloc(sizeof(struct ext2_inode));

	int counter = 0;
	int z = 0;

	int completed_files = 0;
	int firstRun = 1;

	for (unsigned int i = 0; i < (uint)nextSpot; i++)
	{ // iterate over every jpg inode

		counter = nextSpot % inodes_per_block;

		// update z if we've entered a new itable block
		if(firstRun)
		{
			firstRun = 0;
		}
		else if (counter == 0)
		{
			z++;
			counter = 0;
		}

		struct ext2_group_desc group;
		read_group_desc(fd, z, &group);

		off_t inodeStart = locate_inode_table(z, &group);

		read_inode(fd, z, inodeStart, jpegs[i], jpgInode); // acquire the current .jpg inode from the table

		for (unsigned int j = 0; j < (uint)dirNextSpot; j++)
		{ // iterate over every directory inode

			read_inode(fd, z, inodeStart, dirs[j], dirInode); // acquire the next directory inode from the table

			unsigned char buffer[1024];
			struct ext2_dir_entry_2 *currEntry;

			lseek(fd, BLOCK_OFFSET((z * blocks_per_group) + dirInode->i_block[0]), SEEK_SET);
			read(fd, buffer, 1024); // read block from disk

			int offset = 0;
			while ((unsigned int)offset < dirInode->i_size) // iterate over the directory entries to find the desired file name
			{

				currEntry = (struct ext2_dir_entry_2 *)&(buffer[offset]);

				if (currEntry->inode == (unsigned int)jpegs[i])
				{ // the desired inode was found, so quit the loop

					int name_len = currEntry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
					char name[EXT2_NAME_LEN];
					strncpy(name, currEntry->name, name_len);

					break; // currEntry now points to the correct file name
				}
				else if((int)currEntry->inode == 0)
				{
					break; // there are no more entries in this directory, so on to next
				}

				unsigned int length = currEntry->name_len;

				if (((int)length % 4) == 0)
				{
					offset += (8 + length);
				}
				else
				{
					offset += (8 + length + (4 - (length % 4))); // point the offset to the start of the next possible entry
				}
			}

			if (((unsigned int)jpegs[i]) == currEntry->inode)
			{ // the file name for the current jpg has been found!

				char dir[50];  // outputDir/
				char dir1[50]; // outputDir/

				char file[255]; // filename.jpg
				char inode[12]; // file-inode#.jpg

				char file_name[255];	  // outputDir/filename.jpg
				char inode_file_name[12]; // outputDir/inode#.jpg

				memcpy(file, currEntry->name, currEntry->name_len); // get the name of the current jpg file
				file[currEntry->name_len] = '\0';

				// get the inode file name of the current jpg
				snprintf(inode, 12, "file-%d", jpegs[i]);
				char *jpg2 = ".jpg";
				strcat(inode, jpg2);

				// add the dir name to the inode file
				sprintf(dir1, "%s/", outP2);
				strcat(dir1, inode);
				strcpy(inode_file_name, dir1);

				// create a copy of the inode file so it is not corrupted below
				char *inodeFile = malloc(sizeof(inode_file_name));
				strcpy(inodeFile, inode_file_name);

				// add dir name to the file name
				sprintf(dir, "%s/", outP3); // PROBLEM HERE
				strcat(dir, file);
				strcpy(file_name, dir);

				copyFile(inodeFile, file_name);

				free(inodeFile);

				if (++completed_files == nextSpot)
				{
					free(jpgInode);
					free(dirInode);
					close(fd);
					exit(0);
				}

				break; // on to the next jpg entry!
			}
		}
	}

	free(jpgInode);
	free(dirInode);

	close(fd);
}
