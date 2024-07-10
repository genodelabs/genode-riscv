# A place to store RISC-V board support code for Genode

While the generic RISC-V implementation is located within [Genode's main
repository](https://github.com/genodelabs/genode.git), this is the place where
board specific code outside of Genode's mainline is located.

To use this repository, you first need to obtain a clone of Genode:

```sh
$ git clone https://github.com/genodelabs/genode.git genode
```

Now clone the `genode-riscv.git` repository to `genode/repos/riscv`:

```sh
$ git clone https://github.com/ssumpf/genode-riscv.git genode/repos/riscv
```

For building a board specification of the `riscv` repository, the build-directory
configuration `etc/build.conf` must be extended with the following line:

```sh
$ REPOSITORIES += $(GENODE_DIR)/repos/riscv
```

To build a specific board, the `BOARD` variable must be set either in
`etc/build.conf` or at the command line, e.g:

```sh
$ make BOARD=migv core bootstrap
```

Currently available boards: `migv`
