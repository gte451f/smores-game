import datetime
import json
import os
import re
import sys

STOPWORDS = {"a", "an", "the", "to", "for", "and", "or", "of", "in", "on", "with", "is", "this", "that"}


def slugify(first_line: str) -> str:
    first_line = re.sub(r"^#+\s*", "", first_line)
    words = re.findall(r"[a-zA-Z0-9]+", first_line.lower())
    significant = [w for w in words if w not in STOPWORDS] or words
    return "-".join(significant[:4]) or "plan"


def main() -> None:
    data = json.load(sys.stdin)
    plan = (data.get("tool_input") or {}).get("plan", "")
    if not plan.strip():
        return

    first_line = next((l.strip() for l in plan.splitlines() if l.strip()), "plan")
    slug = slugify(first_line)

    now = datetime.datetime.now()
    date_str = f"{now.month}-{now.day}-{now.year % 100}"

    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(os.path.dirname(script_dir))
    out_dir = os.path.join(project_root, "Design", "plans")
    os.makedirs(out_dir, exist_ok=True)

    base_name = f"{date_str}-{slug}"
    path = os.path.join(out_dir, base_name + ".md")
    n = 2
    while os.path.exists(path):
        path = os.path.join(out_dir, f"{base_name}-{n}.md")
        n += 1

    with open(path, "w", encoding="utf-8") as f:
        f.write(plan.rstrip() + "\n")

    print(json.dumps({"systemMessage": f"Saved approved plan to {path}"}))


if __name__ == "__main__":
    main()
