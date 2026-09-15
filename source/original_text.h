/* Extracted DS Style v7.4 pure title functions (Apache-2.0). */
typedef unsigned u32;
typedef char TCHAR;
#define DSTEXT_NO_RECENT_GAME "No recent game"
static void Launcher_CleanTitle(const TCHAR *src, char *dst, u32 dst_size)
{
	u32 i;
	u32 j = 0;
	u32 paren = 0;
	u32 bracket = 0;
	char temp[128];
	char *dot;

	if(dst_size == 0)
		return;

	memset(temp, 0, sizeof(temp));
	strncpy(temp, src, sizeof(temp) - 1);

	dot = strrchr(temp, '.');
	if(dot)
		*dot = '\0';

	for(i = 0; temp[i] != '\0' && j < dst_size - 1; i++)
	{
		if(temp[i] == '(')
		{
			paren = 1;
			continue;
		}
		if(temp[i] == ')')
		{
			paren = 0;
			continue;
		}
		if(temp[i] == '[')
		{
			bracket = 1;
			continue;
		}
		if(temp[i] == ']')
		{
			bracket = 0;
			continue;
		}

		if(!paren && !bracket)
			dst[j++] = temp[i];
	}
	dst[j] = '\0';

	while(j > 0 && (dst[j - 1] == ' ' || dst[j - 1] == '\t'))
	{
		dst[--j] = '\0';
	}

	if(dst[0] == '\0')
	{
		strncpy(dst, temp, dst_size - 1);
		dst[dst_size - 1] = '\0';
	}
}

static int Launcher_SplitTitle(const char *title, char lines[3][32])
{
	int title_len;
	int pos = 0;
	int line_count = 0;
	int i;

	memset(lines, 0, sizeof(char) * 3 * 32);
	title_len = strlen(title);

	while(pos < title_len && line_count < 3)
	{
		int remaining = title_len - pos;
		int max_take = (line_count < 2) ? 20 : 24;
		int take = (remaining > max_take) ? max_take : remaining;
		int split = pos + take;

		if(split < title_len)
		{
			for(i = split; i > pos + 8; i--)
			{
				if(title[i] == ' ')
				{
					split = i;
					break;
				}
			}
		}

		if(split <= pos)
			split = pos + take;

		strncpy(lines[line_count], title + pos, split - pos);
		lines[line_count][split - pos] = '\0';

		while(lines[line_count][0] == ' ')
			memmove(lines[line_count], lines[line_count] + 1, strlen(lines[line_count]));

		pos = split;
		while(title[pos] == ' ')
			pos++;

		line_count++;
	}

	if(pos < title_len && line_count > 0)
	{
		int last = line_count - 1;
		int len = strlen(lines[last]);
		if(len > 21)
			len = 21;
		while(len > 0 && lines[last][len - 1] == ' ')
			len--;
		lines[last][len] = '\0';
		strcat(lines[last], "...");
	}

	if(line_count == 0)
	{
		strcpy(lines[0], " ");
		line_count = 1;
	}

	return line_count;
}

static int Launcher_SplitStartTitle(const char *title, char lines[3][32])
{
    int title_len;
    int pos = 0;
    int line_count = 0;
    int i;
    const int max_take = 18;

    memset(lines, 0, sizeof(char) * 3 * 32);
    if(!title || !title[0])
    {
        snprintf(lines[0], 32, "%s", DSTEXT_NO_RECENT_GAME);
        return 1;
    }

    title_len = strlen(title);
    while(pos < title_len && line_count < 3)
    {
        int remaining = title_len - pos;
        int take = (remaining > max_take) ? max_take : remaining;
        int split = pos + take;

        if(split < title_len)
        {
            for(i = split; i > pos + 5; i--)
            {
                if(title[i] == ' ')
                {
                    split = i;
                    break;
                }
            }
        }

        if(split <= pos)
            split = pos + take;

        strncpy(lines[line_count], title + pos, split - pos);
        lines[line_count][split - pos] = '\0';

        while(lines[line_count][0] == ' ')
            memmove(lines[line_count], lines[line_count] + 1, strlen(lines[line_count]));

        pos = split;
        while(title[pos] == ' ')
            pos++;

        line_count++;
    }

    if(pos < title_len && line_count > 0)
    {
        int last = line_count - 1;
        int len = strlen(lines[last]);
        if(len > 15)
            len = 15;
        while(len > 0 && lines[last][len - 1] == ' ')
            len--;
        lines[last][len] = '\0';
        strcat(lines[last], "...");
    }

    if(line_count == 0)
    {
        strcpy(lines[0], " ");
        line_count = 1;
    }

    return line_count;
}
