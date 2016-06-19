/*
 * sod: simple environment module system
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <libgen.h>
#include <unistd.h>

#include <sqlite3.h>

#include <solv/pool.h>
#include <solv/poolarch.h>
#include <solv/repo.h>
#include <solv/solver.h>
#include <solv/selection.h>
#include <solv/solverdebug.h>
#include <solv/testcase.h>

Id SOLVABLE_SCRIPT = 0;
const char *usage[] = {
    "Usage: sod [OPTION]... [NAME]...",
    NULL
};

void
error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "sod: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

void
echo(const char *fmt, ...)
{
    // TBD: escape quotes
    va_list args;
    va_start(args, fmt);
    printf("echo ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

typedef struct {
    const char *ptr;
    int len;
} substring_t;

char *
repo_name_from_filename(const char *filename)
{
    const char *a = strrchr(filename, '/');
    a = (a) ? a + 1 : filename;
    if (!*a) return 0;
    const char *b = strchr(a, '.');
    return (b) ? strndup(a, b-a) : strdup(a);
}

#define FORTOKEN(TOK, STR, SEP) \
    for (char *_=0, *TOK=strtok_r(STR,SEP,&_); TOK; TOK=strtok_r(0,SEP,&_))

Repo *
repo_add(Pool *pool, const char *filename)
{
    char *name = repo_name_from_filename(filename);
    if (!name) error("%s is a directory", filename);
    Repo *repo = repo_create(pool, name);
    Repodata *data = repo_add_repodata(repo, 0);
    free(name);

    sqlite3 *db = NULL;
    sqlite3_open_v2(filename, &db, SQLITE_OPEN_READONLY, NULL);
    sqlite3_stmt *stmt = NULL;
    const char sql[] = "select name,version||'-'||release as evr,arch,provides,requires,summary,script from packages";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char
            *pkg = (const char *)sqlite3_column_text(stmt, 0),
            *evr = (const char *)sqlite3_column_text(stmt, 1),
            *arch = (const char *)sqlite3_column_text(stmt, 2);

        Solvable *s = pool_id2solvable(pool, repo_add_solvable(repo));
        s->name = pool_str2id(pool, pkg, 1);
        s->evr = pool_str2id(pool, evr, 1);
        s->arch = pool_str2id(pool, arch, 1);

        // pkg provides itself, 'name = evr'
        s->provides = repo_addid_dep(repo, s->provides,
            pool_rel2id(pool, s->name, s->evr, REL_EQ, 1), 0);

        const char *provides = (const char *)sqlite3_column_text(stmt, 3);
        if (provides && *provides) {
            char *tmpstr = strdup(provides);
            FORTOKEN(pro, tmpstr, ";") {
                s->provides = repo_addid_dep(repo, s->provides,
                    testcase_str2dep(pool, pro), 0);
            }
            free(tmpstr);
        }

        const char *requires = (const char *)sqlite3_column_text(stmt, 4);
        if (requires && *requires) {
            char *tmpstr = strdup(requires);
            FORTOKEN(req, tmpstr, ";") {
                s->requires = repo_addid_dep(repo, s->requires,
                    testcase_str2dep(pool, req), -SOLVABLE_PREREQMARKER);
            }
            free(tmpstr);
        }

        // summary
        const char *summary = (const char *)sqlite3_column_text(stmt, 5);
        if (summary && *summary)
            repodata_set_str(data, s-pool->solvables, SOLVABLE_SUMMARY, summary);

        // script
        const char *script = (const char *)sqlite3_column_text(stmt, 6);
        if (script && *script)
            repodata_set_str(data, s-pool->solvables, SOLVABLE_SCRIPT, script);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    repodata_free_dircache(data);
    repodata_internalize(data);
    //testcase_write_testtags(repo, stdout);
    return repo;
}

void
solvable_copy(Solvable *s1, Solvable *s2)
{
    if (!s1 || !s2) return;

    s2->name   = s1->name;
    s2->arch   = s1->arch;
    s2->evr    = s1->evr;
    s2->vendor = s1->vendor;

#define ADDIDS(FIELD) \
    if (s1->FIELD) \
        for (Id *idp = s1->repo->idarraydata + s1->FIELD; *idp; idp++) \
            s2->FIELD = repo_addid(s2->repo, s2->FIELD, *idp);
    ADDIDS(provides)
    ADDIDS(obsoletes)
    ADDIDS(conflicts)
    ADDIDS(requires)
    ADDIDS(recommends)
    ADDIDS(suggests)
    ADDIDS(supplements)
    ADDIDS(enhances)
#undef ADDIDS
}

typedef enum {
    SOD_OP_ADD = '+',
    SOD_OP_SET = '=',
} op_t;

typedef struct {
    op_t op;
    substring_t key;
    substring_t val;
} cmd_t;

void
print_cmd(const char *func, cmd_t *cmd)
{
    printf("%s %.*s '%.*s\'\n", func,
        cmd->key.len, cmd->key.ptr,
        cmd->val.len, cmd->val.ptr);
}

void
do_cmd(cmd_t *cmd)
{
    switch (cmd->op) {
    case SOD_OP_ADD:
        print_cmd("__sod_push", cmd);
        break;
    case SOD_OP_SET:
        print_cmd("__sod_set", cmd);
        break;
    }
}

void
undo_cmd(cmd_t *cmd)
{
    switch (cmd->op) {
    case SOD_OP_ADD:
        print_cmd("__sod_pop", cmd);
        break;
    case SOD_OP_SET:
        print_cmd("__sod_unset", cmd);
        break;
    }
}

cmd_t *
parse_script(const char *script, int *ncmds)
{
    *ncmds = 0;
    if (!script) return NULL;
    size_t maxcmds = 4;
    cmd_t *cmds = (cmd_t *)malloc(maxcmds*sizeof(cmd_t));

    const char *s = script;
    while (*s) {
        while (*s == ' ') ++s; // skip spaces

        char *k = strchr(s, ' ');
        assert(k);
        while (*++k == ' '); // skip spaces

        char *v = strchr(k, ' ');
        assert(v);
        int nk = v - k;
        while (*++v == ' '); // skip spaces

        char *n = strchr(v, '\n');
        if (!n) n = strchr(v, 0);
        int nv = n - v;
        while (*n == '\n') ++n; // skip newlines

        if (*ncmds == maxcmds) {
            maxcmds *= 2;
            cmds = (cmd_t *)realloc(cmds, maxcmds*sizeof(cmd_t));
        }

        cmd_t *cmd = cmds + *ncmds;
        *ncmds += 1;
        cmd->op = *s;
        cmd->key = (substring_t){ k, nk };
        cmd->val = (substring_t){ v, nv };

        s = n;
    }

    return cmds;
}

enum sod_mode {
    NONE = 0,
    AVAIL,
    LIST,
    LOAD,
    PURGE,
    SWAP,
    UNLOAD
};

struct sod_mode_name {
    const char *name;
    enum sod_mode mode;
};

int
main(int argc, char *argv[])
{
    char *repos = getenv("__sod_repos");
    char *installed = getenv("__sod_installed");
    int verbose = 0;

    const char *shortopts = "ehiI:r:qvV";
    const struct option longopts[] = {
        { "help",      no_argument,       0, 'h' },
        { "installed", required_argument, 0, 'I' },
        { "repos",     required_argument, 0, 'r' },
        { "verbose",   no_argument,       0, 'v' },
        { "version",   no_argument,       0, 'V' },
        { 0, 0, 0, 0 }
    };

    opterr = 0; // suppress getopt warning messages
    while (1) {
        int lastind = optind;
        int c = getopt_long(argc, argv, shortopts, longopts, 0);
        if (c == -1) break;
        switch (c) {
        case 'h':
            for (const char **line = usage; *line; line++)
                echo("%s", *line);
            return 0;
        case 'I':
            installed = optarg;
            break;
        case 'r':
            repos = optarg;
            break;
        case 'v':
            verbose++;
            break;
        case 'V':
            echo("sod 0.0.0");
            return 0;
        case '?':
        case ':':
            if (optopt) {
                for (const struct option *o = longopts; o->val; ++o) {
                    if (optopt == o->val) {
                        if (strchr(shortopts, o->val)) {
                            error("option `-%c/--%s` is missing an argument",
                                    o->val, o->name);
                        } else {
                            error("option `--%s` is missing an argument",
                                    o->name);
                        }
                    }
                }
                error("unknown option `-%c`", optopt);
            } else {
                error("unknown option `%s`", argv[lastind]);
            }
            break;
        }
    }

    if (!repos) error("no repo specified");

    if (optind == argc) return 0; // nothing to do
    const char *mode_arg = argv[optind++];
    enum sod_mode mode = NONE;

    const struct sod_mode_name mode_names[] = {
        { "add",       LOAD },
        { "avail",     AVAIL },
        { "erase",     UNLOAD },
        { "list",      LIST },
        { "load",      LOAD },
        { "install",   LOAD },
        { "purge",     PURGE },
        { "rm",        UNLOAD },
        { "swap",      SWAP },
        { "uninstall", UNLOAD },
        { "unload",    UNLOAD },
        { 0 }
    };

    for (const struct sod_mode_name *x = mode_names; x->mode; x++) {
        if (strcmp(mode_arg, x->name) == 0) {
            mode = x->mode;
            break;
        }
    }
    if (!mode) error("unknown command: %s", mode_arg);

    // create & initialize pool
    Pool *pool = pool_create();
    pool->debugmask |= SOLV_DEBUG_TO_STDERR;
    pool_setdebuglevel(pool, verbose);
    pool_setarch(pool, 0); // unset
    /* global */ SOLVABLE_SCRIPT = pool_str2id(pool, "solvable:script", 1);
    Id SOLVABLE_SOURCEID = pool_str2id(pool, "solvable:sourceid", 1);

    // add repos
    char *tmpstr = strdup(repos);
    FORTOKEN(filename, tmpstr, ":") {
        Repo *repo = repo_add(pool, filename);
        repo->priority = 0;
        repo->subpriority = 0;
    }
    free(tmpstr);

    // construct installed repo
    Repo *installed_repo = repo_create(pool, "installed");
    Repodata *data = repo_add_repodata(installed_repo, 0);
    pool_set_installed(pool, installed_repo);
    pool_addfileprovides(pool);
    pool_createwhatprovides(pool);
    FORTOKEN(pkg, (installed) ? strdup(installed) : 0, ":") {
        // Have to create new Solvable *before* searching because Solvable
        // pointers are invalidated when creating/deleting other Solvables.
        Id p2 = repo_add_solvable(installed_repo);
        Solvable *s2 = pool_id2solvable(pool, p2);
        Id p = testcase_str2solvid(pool, pkg);
        if (!p) error("can't find '%s'", pkg);
        Solvable *s = pool_id2solvable(pool, p);
        solvable_copy(s, s2);
        repodata_set_id(data, p2, SOLVABLE_SOURCEID, p);
    }
    repodata_free_dircache(data);
    repodata_internalize(data);

    pool_addfileprovides(pool);
    pool_createwhatprovides(pool);

    Queue jobs, repofilter;
    queue_init(&jobs);

    if (optind < argc) {
        int selflags = SELECTION_NAME | SELECTION_PROVIDES | SELECTION_CANON |
                       SELECTION_DOTARCH | SELECTION_REL | SELECTION_GLOB;
        if (mode == LIST)
            selflags |= SELECTION_INSTALLED_ONLY;

        Queue sel;
        queue_init(&sel);
        for (; optind < argc; optind++) {
            const char *arg = argv[optind];
            selection_make(pool, &sel, arg, selflags);
            if (!sel.count) error("no match for '%s'", arg);
            for (int i = 0; i < sel.count; i++) {
                queue_push(&jobs, sel.elements[i]);
            }
            queue_empty(&sel);
        }
        queue_free(&sel);

    } else if (mode == AVAIL || mode == PURGE) {
        queue_push2(&jobs, SOLVER_SOLVABLE_ALL, 0);

    } else if (mode == LIST) {
        queue_init(&repofilter);
        queue_push2(&jobs, SOLVER_SOLVABLE_ALL, 0);
        queue_push2(&repofilter, SOLVER_SOLVABLE_REPO | SOLVER_SETREPO,
            pool->installed->repoid);
        selection_filter(pool, &jobs, &repofilter);
    }

    if (mode == AVAIL || mode == LIST) {
        Queue q;
        queue_init(&q);
        for (int i = 0; i < jobs.count; i += 2) {
            pool_job2solvables(pool, &q, jobs.elements[i], jobs.elements[i+1]);
            for (int j = 0; j < q.count; j++) {
                Id p = q.elements[j];
                Solvable *s = pool_id2solvable(pool, p);
                if (mode == AVAIL && s->repo == pool->installed)
                    continue;
                const char *str = pool_solvable2str(pool, s);
                  //  *sum = solvable_lookup_str(s, SOLVABLE_SUMMARY);
                echo("%s", str); // TBD: sort
            }
            queue_empty(&q);
        }
        queue_free(&q);
        return 0;
    }

    Solver *solv = solver_create(pool);
    solver_set_flag(solv, SOLVER_FLAG_ALLOW_NAMECHANGE, 1);
    if (mode == PURGE || mode == SWAP)
        solver_set_flag(solv, SOLVER_FLAG_ALLOW_UNINSTALL, 1);

    int how = (mode == PURGE) ? 0 : SOLVER_SOLVABLE_NAME;
    switch (mode) {
    case LOAD:
    case SWAP:
        how |= SOLVER_INSTALL;
        break;
    case PURGE:
    case UNLOAD:
        how |= SOLVER_ERASE;
        break;
    default:
        error("internal error %d", mode);
    }

    for (int i = 0; i < jobs.count; i += 2) {
        jobs.elements[i] |= how;
    }

    int nprobs = solver_solve(solv, &jobs);
    if (nprobs)
    {
        if (nprobs > 1) fprintf(stderr, "Found %d probs:\n", nprobs);
        for (int prob = 1; prob <= nprobs; prob++)
        {
            fprintf(stderr, "Problem");
            if (nprobs > 1) fprintf(stderr, " %d", prob);
            fprintf(stderr, ":\n");
            solver_printprobleminfo(solv, prob);
            fprintf(stderr, "\n");
            int nsols = solver_solution_count(solv, prob);
            for (int sol = 1; sol <= nsols; sol++)
            {
                fprintf(stderr, "Solution");
                if (nsols > 1) fprintf(stderr, " %d", sol);
                fprintf(stderr, ":\n");
                solver_printsolution(solv, prob, sol);
                if (sol < nsols) fprintf(stderr, "\n");
            }
        }
    }
    else
    {
        Transaction *trans = solver_create_transaction(solv);
        transaction_order(trans, 0);

        for (int j = 0; j < trans->steps.count; j++)
        {
            Id p = trans->steps.elements[j];
            Id type = transaction_type(trans, p, SOLVER_TRANSACTION_RPM_ONLY);
            Solvable *s = pool_id2solvable(pool, p);
            if (type == SOLVER_TRANSACTION_ERASE) {
                p = solvable_lookup_id(s, SOLVABLE_SOURCEID);
                s = pool_id2solvable(pool, p);
            }
            const char *str = testcase_solvid2str(pool, p);
            const char *script = solvable_lookup_str(s, SOLVABLE_SCRIPT);

            int ncmds = 0;
            cmd_t *cmds = parse_script(script, &ncmds);
            switch (type) {
            case SOLVER_TRANSACTION_ERASE:
                echo("unloading %s", str);
                for (int i = ncmds - 1; i >= 0; i--)
                    undo_cmd(cmds + i);
                printf("__sod_pop __sod_installed '%s'\n", str);
                break;
            case SOLVER_TRANSACTION_INSTALL:
                echo("loading %s", str);
                for (int i = 0; i < ncmds; i++)
                    do_cmd(cmds + i);
                printf("__sod_push __sod_installed '%s'\n", str);
                break;
            }
            free(cmds);
        }

        transaction_free(trans);
    }

    queue_free(&jobs);
    solver_free(solv);
    pool_free(pool);
    return 0;
}
