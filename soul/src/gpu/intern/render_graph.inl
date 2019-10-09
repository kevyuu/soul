namespace Soul { namespace GPU {	
	template<typename T, typename Setup, typename Execute>
	GraphicNode<T, Execute>& RenderGraph::addGraphicPass(const char* name, Setup setup, Execute&& execute) {
		using Node = GraphicNode<T, Execute>;
		Node* node = (Node*) malloc(sizeof(Node));
		new(node) Node(std::forward<Execute>(execute));
		node->name = name;
		passNodes.add(node);
		setup(this, &node->pipelineDesc, &node->data);
		return *node;
	}

	template<typename T, typename Setup, typename Execute>
	TransferNode<T, Execute>& RenderGraph::addTransferPass(const char* name, Setup setup, Execute&& execute) {
		using Node = TransferNode<T, Execute>;
		Node* node = (Node*) malloc(sizeof(Node));
		new(node) Node(std::forward<Execute>(execute));
		passNodes.add(node);
		node->name = name;
		setup(this, &node->data);
		return *node;
	}

	/*template<typename T, typename Setup, typename Execute>
	ComputeNode<T, Execute>& RenderGraph::addComputePass(const char* name, Setup setup, Execute&& execute) {
		currentPassNodeID = passNodes.size();
		using Node = TransferNode<T, Execute>;
		Node* node = (Node*) malloc(sizeof(Node));
		new(node) Node(std::forward(execute));
		passNodes.add(node);
		node->name = name;
		setup(this, &node->pipelineDesc, &node->data);
		return *node;
	}*/
}}