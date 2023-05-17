"BACI CODEOWNERS check"

import argparse
import re
import common_utils as utils
from codeowners import CodeOwners


def fix_codeowner_pattern(content):
    owner = CodeOwners(content)

    for i, (pattern, path, owners, line_number, section_name) in enumerate(owner.paths):
        if pattern.pattern.startswith(r"\A"):
            if path.startswith("/"):
                # path with anchor is given in codeowners file -> only match files at exact location
                new_pattern = re.compile(pattern.pattern.replace(r"\A", r"(?:\A|\A/)"))
            else:
                # path without anchor is given in codeowners file -> also match files in subfolder
                new_pattern = re.compile(pattern.pattern.replace(r"\A", r"(?:\A|/)"))

            new_entry = (new_pattern, path, owners, line_number, section_name)
            owner.paths[i] = new_entry  # replace old entry with fixed version

    return owner


def check_codeowners_rules_are_used(look_cmd, allerrors):
    codeowners_file = ".gitlab/CODEOWNERS"
    with open(codeowners_file, "r") as file:
        codeowners_content = file.read()
    owner = fix_codeowner_pattern(codeowners_content)
    pathlist = list()
    for i in owner.paths:
        pathlist.append(i[1])
    for ff in utils.files_changed(look_cmd):
        for i in owner.matching_lines(ff):
            try:
                pathlist.remove(i[2])
            except ValueError:
                ()

    if len(pathlist) > 0:
        if len(allerrors) > 0:
            allerrors.append("")
        allerrors.append(
            "The following CODEOWNER rules do not represent files or directories:"
        )
        allerrors += pathlist
    return len(pathlist)


def check_files_are_owned(look_cmd, allerrors):
    codeowners_file = ".gitlab/CODEOWNERS"
    with open(codeowners_file, "r") as file:
        codeowners_content = file.read()
    owner = fix_codeowner_pattern(codeowners_content)
    files_not_in_CO = [
        ff
        for ff in utils.files_changed(look_cmd)
        if owner.matching_line(ff)[2] == "*" and ff != ""
    ]
    if len(files_not_in_CO) > 0:
        if len(allerrors) > 0:
            allerrors.append("")
        allerrors.append("The following files do not exist in the CODEOWNERS file:")
        allerrors += files_not_in_CO
    return len(files_not_in_CO)


def main():
    # build command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--diff_only",
        action="store_true",
        help="Add this tag if only the difference to HEAD should be analyzed. This flag should be used as a pre-commit hook. Otherwise all files are checked.",
    )
    parser.add_argument(
        "--out",
        type=str,
        default=None,
        help="Add this tag if the error message should be written to a file.",
    )
    args = parser.parse_args()
    diff_only = args.diff_only
    # error file (None for sys.stderr)
    errfile = args.out
    errors = 0
    allerrors = []
    try:
        if diff_only:
            look_cmd = "git diff --name-only --cached --diff-filter=MRAC"
        else:
            look_cmd = "git ls-files"
            errors += check_codeowners_rules_are_used(look_cmd, allerrors)
        errors += check_files_are_owned(look_cmd, allerrors)
    except ValueError:
        print("Something went wrong! Check the error functions in this script again!")
        errors += 1
    utils.pretty_print_error_report("", allerrors, errfile)
    return errors


if __name__ == "__main__":
    import sys

    sys.exit(main())
