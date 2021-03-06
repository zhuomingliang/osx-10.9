/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright (C) 1995, 1996, 1997 Wolfgang Solfrank
 * Copyright (c) 1995 Martin Husemann
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Martin Husemann
 *	and Wolfgang Solfrank.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "ext.h"
#include "fsutil.h"

int
checkfilesys(fname)
	const char *fname;
{
	int dosfs;
	struct bootblock boot;
	int finish_dosdirsection=0;
	int mod = 0;
	int ret = 8;
	int tryFatalAgain = 1;
	int tryErrorAgain = 3;
	int tryOthersAgain = 3;

	rdonly = alwaysno || quick;
	if (!preen)
		printf("** %s", fname);

	dosfs = open(fname, rdonly ? O_RDONLY : O_RDWR | O_EXLOCK, 0);
	if (dosfs < 0 && !rdonly) {
		dosfs = open(fname, O_RDONLY, 0);
		if (dosfs >= 0)
			pwarn(" (NO WRITE)\n");
		else if (!preen)
			printf("\n");
		rdonly = 1;
	} else if (!preen)
		printf("\n");

	if (dosfs < 0) {
		perr("Can't open");
		return 8;
	}

	mod = readboot(dosfs, &boot);
	if (mod & FSFATAL) {
		close(dosfs);
		return 8;
	}
	
	if (quick) {
		/*
		 * FAT12 volumes don't have a dirty bit.  If we were asked for
		 * a quick check, then actually do a full scan without repairs.
		 */
		if (boot.ClustMask == CLUST12_MASK) {
			/* Don't attempt to do any repairs */
			rdonly = 1;
			alwaysno = 1;
			alwaysyes = 0;
			quiet = 1;
			
			/* Go verify the volume */
			goto Again;
		}
		else if (isdirty(dosfs, &boot, boot.ValidFat >= 0 ? boot.ValidFat : 0)) {
			pwarn("FILESYSTEM DIRTY; SKIPPING CHECKS\n");
			ret = FSDIRTY;
		}
		else {
			pwarn("FILESYSTEM CLEAN; SKIPPING CHECKS\n");
			ret = 0;
		}
		
		close(dosfs);
		return ret;
	}

Again:
	mod = 0;		/* make sure its reset */
	boot.NumFiles = 0;	/* Reset file count in case we loop back here */
	boot.NumBad = 0;
	
	/*
	 * [2771127] When there was no active FAT, this code used to
	 * compare FAT #0 with all the other FATs.  That doubled the
	 * already large memory usage, and doesn't seem very useful.
	 * Microsoft's specification says the purpose of the alternate
	 * FATs is in case a sector goes bad in the main FAT.  In fact,
	 * a Windows 2000 system never even notices the FATs have
	 * different values.  Besides, the filesystem itself only ever
	 * uses FAT #0.
	 */
	if (!preen && !quiet) {
		printf("** Phase 1 - Preparing FAT\n");
        }

	mod |= fat_init(dosfs, &boot);
	
	if (mod & FSFATAL) {
		close(dosfs);
		return 8;
	}

	if (!preen && !quiet)
		printf("** Phase 2 - Checking Directories\n");

	mod |= resetDosDirSection(&boot);
	finish_dosdirsection = 1;
	if (mod & FSFATAL)
		goto out;
	/* delay writing FATs */

	mod |= handleDirTree(dosfs, &boot);
	if (mod & FSFATAL)
		goto out;

	if (!preen && !quiet)
		printf("** Phase 3 - Checking for Orphan Clusters\n");
	
	/*
	 * Should we skip this if (mod & FSERROR)?  It would be bad to free clusters
	 * that are actually referenced by some directory entry, even if they are
	 * beyond the logical file size (and the file size was not repaired).
	 */
	mod |= fat_free_unused();
	if (mod & FSFATAL)
		goto out;

	if (quick) {
		if (mod) {
			printf("FILESYSTEM DIRTY\n");
			ret = FSDIRTY;
		}
		else {
			printf("FILESYSTEM CLEAN\n");
			ret = 0;
		}
	}

	if (boot.NumBad)
		pwarn("%d files, %lld KiB free (%d clusters), %lld KiB bad (%d clusters)\n",
			  boot.NumFiles,
			  (long long) boot.NumFree * (long long) boot.ClusterSize / 1024LL, boot.NumFree,
			  (long long) boot.NumBad * (long long) boot.ClusterSize / 1024LL, boot.NumBad);
	else
		pwarn("%d files, %lld KiB free (%d clusters)\n",
			  boot.NumFiles,
			  (long long) boot.NumFree * (long long) boot.ClusterSize / 1024LL, boot.NumFree);

	/*
	 * If we repaired everything, offer to mark the file system clean.
	 */
	if (mod && (mod & FSERROR) == 0) {
		if (mod & FSDIRTY) {
			if (ask(1, "MARK FILE SYSTEM CLEAN") == 0)
				mod &= ~FSDIRTY;

			if (mod & FSDIRTY) {
				pwarn("MARKING FILE SYSTEM CLEAN\n");
				mod |= fat_mark_clean();
			} else {
				pwarn("\n***** FILE SYSTEM IS LEFT MARKED AS DIRTY *****\n");
			}
		}
	}

	/*
	 * We're done changing the FAT, so flush any changes.
	 */
	mod |= fat_flush();

	/* Don't bother trying multiple times if we're not doing repairs */
	if (mod && rdonly)
		goto out;

	if ((mod & FSFATAL) && (--tryFatalAgain > 0))
		goto Again;
	if ((mod & FSERROR) && (--tryErrorAgain > 0))
		goto Again;
	if ((mod & FSFIXFAT) && (--tryOthersAgain > 0))
		goto Again;
		
	if (mod & (FSFATAL | FSERROR))
		goto out;

	ret = 0;

    out:
	if (finish_dosdirsection)
		finishDosDirSection();
	fat_uninit();
	freeUseMap();
	close(dosfs);

	if (mod & (FSFATMOD|FSDIRMOD))
		pwarn("\n***** FILE SYSTEM WAS MODIFIED *****\n");

	return ret;
}
