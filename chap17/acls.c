/* acls.c - shows permissions for a given user or group to a file, considering ACLs.
 *
 * Access Control Lists (ACLs) are a means to achieve finer permission control
 * on files than the traditional permissions on owner, group and other. They
 * allow, for example, special permissions to be set to specific user(s) or
 * group(s). Although not oficially described on POSIX or any other standard,
 * ACLs are implemented in many UNIX systems, though with potentially different
 * APIs in each of them. This program was tested to work on Linux systems only.
 *
 * What this program basically does is to apply the ACL permission checking
 * algorithm in a given file:
 *
  *    * If the user is the `root' user, all access is granted, with the exception
 *    of execute permission. In that case, for execute access to be granted,
 *    it must be given to at least one of the entries in the ACL.
 *
 *    * If the user is the file owner (ACL_USER_OBJ), then the permissions for
 *    this entry are granted.
 *
 *    * If the user matches one of the user entries in the ACL (ACL_USER), then
 *    the permissions for that entry are granted, ANDed with the ACL_MASK entry.
 *
 *    * If the user's group or one of his supplementary groups match the group
 *    owner of the file (ACL_GROUP_OBJ) and the permissions for it grant the
 *    requested access, then access is allowed after ANDing with ACL_MASK entry.
 *
 *    * Otherwise, if it matches one of the group entries (ACL_GROUP), and it
 *    grants the requested access, then access is allowed after ANDing with
 *    ALC_MASK entry. Otherwise it is denied.
 *
 *    * If any of the above matched, the permissions granted using the
 *    ACL_OTHER entry.
 *
 * Usage
 *
 *    $ ./acls <u|g> <user|group> <file>
 *    Permissions for user 1000: rw-
 *
 *    <u|g> - determines if the next parameter refers to a user or a group.
 *    <user|group> - user or group identifier. Can be an id or a name.
 *    <file> - the file to be checked.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h> /* NGROUPS_MAX */
#include <sys/acl.h>
#include <acl/libacl.h>
#include <limits.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { FALSE, TRUE } Bool;
typedef enum { USER, GROUP } AclType;

struct st_acl_request {
  AclType type;
  long qualifier;
};

struct st_acl_permissions {
  acl_permset_t permset;
  acl_permset_t mask;
  Bool execute; /* special flag for the root permissions */
};

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static void parseInput(const char *type, const char *identifier, struct st_acl_request *s);
static int  getPermissions(const char *file, const struct st_acl_request request, struct st_acl_permissions *permissions);
static void printPermissions(struct st_acl_request request, struct st_acl_permissions permissions);

int
main(int argc, char *argv[]) {
  if (argc != 4) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  char *type, *identifier, *file;
  struct st_acl_request acl_request;
  struct st_acl_permissions permissions;

  type       = argv[1];
  identifier = argv[2];
  file       = argv[3];

  parseInput(type, identifier, &acl_request);
  getPermissions(file, acl_request, &permissions);

  printPermissions(acl_request, permissions);

  exit(EXIT_SUCCESS);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <u|g> <user|group> <file>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

static uid_t
userIdFromName(const char *name) {
  errno = 0;
  struct passwd *p = getpwnam(name);

  if (errno != 0) {
    pexit("getpwnam");
  } else if (p == NULL) {
    fprintf(stderr, "unknown user: %s\n", name);
    exit(EXIT_FAILURE);
  }

  return p->pw_uid;
}

static gid_t
groupIdFromName(const char *name) {
  errno = 0;
  struct group *g = getgrnam(name);

  if (errno != 0) {
    pexit("getgrnam");
  } else if (g == NULL) {
    fprintf(stderr, "unknown group: %s\n", name);
    exit(EXIT_FAILURE);
  }

  return g->gr_gid;
}

static int
getGroups(const uid_t uid, int size, gid_t *groups) {
  struct group *g;
  int n;
  uid_t cuid;
  long usernameMax;
  char **p;

  n = 0;

  usernameMax = sysconf(_SC_LOGIN_NAME_MAX);
  if (usernameMax == -1) {
    usernameMax = 256; /* make a guess */
  }

  /* point to the beginning of the file to avoid starting on a dirty state */
  setgrent();

  for (g = getgrent(); g != NULL; g = getgrent()) {
    for (p = g->gr_mem; *p != NULL; ++p) {
      cuid = userIdFromName(*p);
      if (uid == cuid) {
        if (n < size) {
          groups[n++] = g->gr_gid;
          break;
        } else {
          errno = ENOMEM;
          return -1;
        }
      }
    }
  }

  endgrent();

  return n;
}

static void
parseInput(const char *type, const char *identifier, struct st_acl_request *request) {
  Bool isUser  = !strncmp(type, "u", 1),
       isGroup = !strncmp(type, "g", 1);
  char *p;
  long idNo = 0;

  /* operation type validation */
  if (!(isUser || isGroup)) {
    fprintf(stderr, "identifier type should be either u or g, got %s\n", type);
    exit(EXIT_FAILURE);
  }

  idNo = strtol(identifier, &p, 10);

  if (isUser) {
    if (p == identifier) {
      /* user name was given */
      idNo = userIdFromName(identifier);

    } else { /* user identifier was given */
      if (idNo == LONG_MIN || idNo == LONG_MAX) {
        pexit("strtol");
      }
    }

    request->type = USER;
  } else {
    if (p == identifier) {
      /* group name was given */
      idNo = groupIdFromName(identifier);

    } else {
      /* group identifier was given */
      if (idNo == LONG_MIN || idNo == LONG_MAX) {
        pexit("strtol");
      }
    }

    request->type = GROUP;
  }

  request->qualifier = idNo;
}

static int
getPermissionsGroup(const char *file, gid_t identifier, struct st_acl_permissions *permissions) {
  acl_t acl;
  acl_entry_t entry;
  acl_tag_t tagType;

  struct stat buf;

  int entryId, res;
  gid_t gowner, *gidp;
  Bool done, ruleFound, maskFound;

  if (stat(file, &buf) == -1) {
    pexit("stat");
  }

  gowner = buf.st_gid;
  done = maskFound = ruleFound = FALSE;

  acl = acl_get_file(file, ACL_TYPE_ACCESS);
  if (acl == (acl_t) NULL) {
    pexit("acl_get_file");
  }

  for (entryId = ACL_FIRST_ENTRY; !done; entryId = ACL_NEXT_ENTRY) {
    res = acl_get_entry(acl, entryId, &entry);

    if (res == 1) {
      /* next entry was found */
      if (acl_get_tag_type(entry, &tagType) == -1) {
        pexit("acl_get_tag_type");
      }

      switch (tagType) {
        case ACL_GROUP_OBJ:
          if (gowner == identifier) {
            /* the requested group is the owner of the file and we found the owner group
             * entry: Get the permissions and return them */
            if (acl_get_permset(entry, &permissions->permset) == -1) {
              pexit("acl_get_permset");
            }

            ruleFound = TRUE;

            if (maskFound) {
              done = TRUE;
            }
          }

          break;

        case ACL_GROUP:
          /* requested group is not file owner, but we found a group entry: check if
           * it matches the requested group */
          gidp = acl_get_qualifier(entry);
          if (gidp == NULL) {
            pexit("acl_get_qualifier");
          }

          if (*gidp == identifier) {
            /* group entry matches requested group: get permissions and return them */
            if (acl_get_permset(entry, &permissions->permset) == -1) {
              pexit("acl_get_permset");
            }

            ruleFound = TRUE;

            if (maskFound) {
              done = TRUE;
            }
          }

          break;

        case ACL_OTHER:
          /* entry for default permissions: save them in case we have not found a specific
           * rule for the requested group yet. If we find one later, the permissions
           * will just be overwritten as they should be */
          if (!ruleFound) {
            if (acl_get_permset(entry, &permissions->permset) == -1) {
              pexit("acl_get_permset");
            }
          }

          break;

        case ACL_MASK:
          if (acl_get_permset(entry, &permissions->mask) == -1) {
            pexit("acl_get_permset");
          }

          maskFound = TRUE;

          break;
      }

    } else if (res == 0) {
      /* no more entries in the ACL */
      done = TRUE;
    } else {
      /* some error ocurred, acl_get_entry returned -1 */
      pexit("acl_get_entry");
    }
  }

  acl_free(acl);

  return 0;
}

static int
getPermissionsUser(const char *file, uid_t identifier, struct st_acl_permissions *permissions) {
  acl_t acl;
  acl_entry_t entry;
  acl_tag_t tagType;
  acl_permset_t tmpPerms;

  struct stat buf;

  int entryId, res, perm;
  gid_t owner, *uidp;
  Bool done, ruleFound, maskFound, root;

  if (stat(file, &buf) == -1) {
    pexit("stat");
  }

  owner = buf.st_uid;
  done = maskFound = ruleFound = FALSE;
  root = (identifier == 0);
  permissions->execute = FALSE;

  acl = acl_get_file(file, ACL_TYPE_ACCESS);
  if (acl == (acl_t) NULL) {
    pexit("acl_get_file");
  }

  for (entryId = ACL_FIRST_ENTRY; !done; entryId = ACL_NEXT_ENTRY) {
    res = acl_get_entry(acl, entryId, &entry);

    if (res == 1) {
      /* next entry was found */
      if (acl_get_tag_type(entry, &tagType) == -1) {
        pexit("acl_get_tag_type");
      }

      if (root && !permissions->execute) {
        if (acl_get_permset(entry, &tmpPerms) == -1) {
          pexit("acl_get_permset");
        }

        perm = acl_get_perm(tmpPerms, ACL_EXECUTE);
        if (perm == 1) {
          permissions->execute = TRUE;
        } else if (perm == -1) {
          pexit("acl_get_perm");
        }
      }

      switch (tagType) {
        case ACL_USER_OBJ:
          if (owner == identifier) {
            /* requested user is the owner of the file, and current entry are
             * permissions for the owner: get them and return */
            if (acl_get_permset(entry, &permissions->permset) == -1) {
              pexit("acl_get_permset");
            }

            ruleFound = TRUE;

            if (!root && maskFound) {
              done = TRUE;
            }
          }

          break;

        case ACL_USER:
          /* user rule found: check if if matches requested user and return
           * the permissions if it does */
          uidp = acl_get_qualifier(entry);
          if (uidp == NULL) {
            pexit("acl_get_qualifier");
          }

          if (*uidp == identifier) {
            if (acl_get_permset(entry, &permissions->permset) == -1) {
              pexit("acl_get_permset");
            }

            ruleFound = TRUE;

            if (!root && maskFound) {
              done = TRUE;
            }
          }

          break;

        case ACL_OTHER:
          /* entry for default permissions: save them in case we have not found a specific
           * rule for the requested group yet. If we find one later, the permissions
           * will just be overwritten as they should be */
          if (!ruleFound) {
            if (acl_get_permset(entry, &permissions->permset) == -1) {
              pexit("acl_get_permset");
            }
          }

          break;

        case ACL_MASK:
          if (acl_get_permset(entry, &permissions->mask) == -1) {
            pexit("acl_get_permset");
          }

          maskFound = TRUE;

          break;
      }
    } else if (res == 0) {
      /* all entries searched */
      done = TRUE;
    } else {
      /* acl_get_entry returned -1 */
      pexit("acl_get_entry");
    }
  }

  if (!ruleFound) {
    /* the user is not the file owner, and there is no specific rule for him/her.
     * Fallback on group (and supplementary groups) permissions */
    int suppGroupsNum, i, perm;
    gid_t groups[NGROUPS_MAX];
    struct st_acl_permissions tmpPerms;

    /* clear permissions from checks above since rules to groups will be applied */
    if (acl_clear_perms(permissions->permset) == -1) {
      pexit("acl_clear_perms");
    }

    suppGroupsNum = getGroups(identifier, NGROUPS_MAX, groups);

    for (i = 0; i < suppGroupsNum; ++i) {
      getPermissionsGroup(file, groups[i], &tmpPerms);

      perm = acl_get_perm(tmpPerms.permset, ACL_READ);
      if (perm == 1) {
        if (acl_add_perm(permissions->permset, ACL_READ) == -1) {
          pexit("acl_add_perm");
        }
      } else if (perm == -1) {
        pexit("acl_get_perm");
      }

      perm = acl_get_perm(tmpPerms.permset, ACL_WRITE);
      if (perm == 1) {
        if (acl_add_perm(permissions->permset, ACL_WRITE) == -1) {
          pexit("acl_add_perm");
        }
      } else if (perm == -1) {
        pexit("acl_get_perm");
      }

      perm = acl_get_perm(tmpPerms.permset, ACL_EXECUTE);
      if (perm == 1) {
        if (acl_add_perm(permissions->permset, ACL_EXECUTE) == -1) {
          pexit("acl_add_perm");
        }
      } else if (perm == -1) {
        pexit("acl_get_perm");
      }
    }
  }

  acl_free(acl);

  return 0;
}

static int
getPermissions(const char *file, const struct st_acl_request request, struct st_acl_permissions *permissions) {
  if (request.type == USER) {
    return getPermissionsUser(file, request.qualifier, permissions);
  } else {
    return getPermissionsGroup(file, request.qualifier, permissions);
  }
}

static void
printPermissions(struct st_acl_request request, struct st_acl_permissions permissions) {
  char permStr[] = "---";
  int perm, maskPerm;

  /* special permissions for the root user */
  if (request.qualifier == 0) {
    permStr[0] = 'r';
    permStr[1] = 'w';

    if (permissions.execute) {
      permStr[2] = 'x';
    }
  } else {
    perm = acl_get_perm(permissions.permset, ACL_READ);
    maskPerm = acl_get_perm(permissions.mask, ACL_READ);
    if (perm == -1) {
      pexit("acl_get_perm");

      /* grant permission if mask does not exist (minimal ACL) or mask
       * allows granting the permission */
    } else if (perm == 1 && (maskPerm == -1 || maskPerm == 1)) {
      permStr[0] = 'r';
    }

    perm = acl_get_perm(permissions.permset, ACL_WRITE);
    maskPerm = acl_get_perm(permissions.mask, ACL_WRITE);
    if (perm == -1) {
      pexit("acl_get_perm");
    } else if (perm == 1 && (maskPerm == -1 || maskPerm == 1)) {
      permStr[1] = 'w';
    }

    perm = acl_get_perm(permissions.permset, ACL_EXECUTE);
    maskPerm = acl_get_perm(permissions.mask, ACL_EXECUTE);
    if (perm == -1) {
      pexit("acl_get_perm");
    } else if (perm == 1 && (maskPerm == -1 || maskPerm == 1)) {
      permStr[2] = 'x';
    }
  }

  printf("Permissions for ");

  if (request.type == USER) {
    printf("user ");
  } else {
    printf("group ");
  }

  printf("%ld: %s\n", request.qualifier, permStr);
}
