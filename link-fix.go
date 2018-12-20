package main

import (
    "fmt"
    blackfriday "gopkg.in/russross/blackfriday.v2"
    // "github.com/davecgh/go-spew/spew"
)

const testContent = `# This is markdown
This is a [link][2]. I'm testing this.

[2]: /hello`

type link struct {
    text string
    title string
    href string
}

func walkTree(root *blackfriday.Node, links []link) []link {
    if root == nil {
        return links
    }

    // process node here
    if root.Type == blackfriday.Link {
        // TODO fix FirstChild to find the text node rather than just assume it's the FirstChild
        l := link{text: string(root.FirstChild.Literal), title: string(root.LinkData.Title), href: string(root.LinkData.Destination)}
        fmt.Printf("Found a link: %s %s %s\n", l.text, l.title, l.href)
        links = append(links, l)
    }

    links = walkTree(root.FirstChild, links)
    n := root.Next
    for n != nil {
        links = walkTree(n, links)
        n = n.Next
    }

    return links
}

func printTree(root *blackfriday.Node, indent string) {
    if root == nil {
        return
    }

    fmt.Printf("%sTYPE: %s\n%sCONTENTS: %s\n\n", indent, root.Type.String(), indent, root.Literal)
    printTree(root.FirstChild, indent + "  ")
    n := root.Next
    for n != nil {
        printTree(n, indent)
        n = n.Next
    }
}

func main() {
    // TODO see WithRefOverride
    md := blackfriday.New()
    // md := blackfriday.New(blackfriday.WithNoExtensions())
    root := md.Parse([]byte(testContent))
    links := walkTree(root, make([]link, 0))
    fmt.Printf("%+v\n", links)
    // printTree(root, "")
}
