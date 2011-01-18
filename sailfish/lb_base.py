class LBSim(object):
    """Describes a specific type of a lattice Boltzmann simulation."""

    kernel_file = "__TEMPLATE_NOT_SET__"

    @classmethod
    def add_options(cls, group):
        group.add_argument('--grid', type=str, help='LB grid')

    @classmethod
    def update_defaults(cls, defaults):
        pass

    def update_context(self, ctx):
        """Updates the context dicitionary containing variables used for
        code generation."""
        pass

    def __init__(self, config):
        self.config = config
